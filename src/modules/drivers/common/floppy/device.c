/*
 * Copyright (c) 2007 The tyndur Project. All rights reserved.
 *
 * This code is derived from software contributed to the tyndur Project
 * by Kevin Wolf.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * ``AS IS'' AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED
 * TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL THE COPYRIGHT HOLDERS OR
 * CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
 * EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 * PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS;
 * OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY,
 * WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR
 * OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF
 * ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include <stdint.h>
#include <stdio.h>
#include <string.h>

#include "cdi.h"
#include "cdi/misc.h"
#include "cdi/io.h"
#include "cdi/dma.h"
#include "cdi/cmos.h"

#include "device.h"

/**
 * TODO-Liste:
 *  - PIO ist nur teilweise und nicht funktionsfaehig implementiert
 *  - Funktionen zum Lesen/Schreiben von Tracks
 *  - Funktionen zum Lesen/Schreiben von Blocks hat noch optimierungspotential
 *  - Abfangen wenn die Diskette gewechselt wird
 *  - Funktion zum Pruefen ob ein Laufwerk existiert
 */

// Eine LBA in eine Adresse als CHS umwandeln
static inline void lba2chs(struct floppy_device* device, uint32_t lba,
    uint8_t* cylinder, uint8_t* head, uint8_t* sector);

// Registerzugriffe auf den Kontroller
static inline uint8_t floppy_read_byte(struct floppy_device* device,
    uint16_t reg);
static inline void floppy_write_byte(struct floppy_device* device,
    uint16_t reg, uint8_t value);

// Daten vom Kontroller holen und an ihn senden
static int floppy_read_data(struct floppy_device* device, uint8_t* dest);
static int floppy_write_data(struct floppy_device* device, uint8_t data);

static int floppy_reset_controller(struct floppy_device* device);

static int floppy_drive_motor_set(struct floppy_device* device, int state);
static int floppy_drive_select(struct floppy_device* device);
static int  floppy_drive_set_dr(struct floppy_device* device);
static int floppy_drive_int_sense(struct floppy_device* device);
static void floppy_drive_specify(struct floppy_device* device);
//static void floppy_dma_state_set(struct floppy_device* device, int state);
static int floppy_drive_seek(struct floppy_device* device, uint8_t cylinder,
    uint8_t header);
static int floppy_drive_sector_read(struct floppy_device* device, uint32_t lba,
    void* buffer);
static int floppy_drive_sector_write(struct floppy_device* device, uint32_t lba,
    void* buffer);

// Umgehen mit IRQs
static void floppy_reset_irqcnt(struct floppy_device* device);
static int floppy_wait_irq(struct floppy_device* device, uint32_t timeout);


/**
 * Eine LBA in eine Adresse als CHS umwandeln
 */
static inline void lba2chs(struct floppy_device* device, uint32_t lba,
    uint8_t* cylinder, uint8_t* head, uint8_t* sector)
{
    *sector = (lba % FLOPPY_SECTORS_PER_TRACK(device)) + 1;
    *cylinder = (lba / FLOPPY_SECTORS_PER_TRACK(device)) /
        FLOPPY_HEAD_COUNT(device);
    *head = (lba / FLOPPY_SECTORS_PER_TRACK(device)) %
        FLOPPY_HEAD_COUNT(device);
}

/**
* Byte aus einem Register des Kontrollers lesen
*/
static inline uint8_t floppy_read_byte(struct floppy_device* device,
    uint16_t reg)
{
    return cdi_inb(device->controller->ports[reg]);
}

/**
 * Byte in ein Register des Kontrollers schreiben
 */
static inline void floppy_write_byte(struct floppy_device* device,
    uint16_t reg, uint8_t value)
{
    cdi_outb(device->controller->ports[reg], value);
}

/**
 * Datenbyte vom Kontroller holen
 */
static int floppy_read_data(struct floppy_device* device, uint8_t* dest)
{
    int i;
    uint8_t msr;

    // Wenn der Controller beschaeftigt ist, wird ein bisschen gewartet und
    // danach nochmal probiert
    for (i = 0; i < FLOPPY_READ_DATA_TRIES(device); i++) {
        msr = floppy_read_byte(device, FLOPPY_REG_DSR);

        // Pruefen ob der Kontroller bereit ist, und Daten zum abholen
        // bereitliegen.
        if ((msr & (FLOPPY_MSR_RQM | FLOPPY_MSR_DIO)) == (FLOPPY_MSR_RQM |
            FLOPPY_MSR_DIO))
        {
            *dest = floppy_read_byte(device, FLOPPY_REG_DATA);
            return 0;
        }
        cdi_sleep_ms(FLOPPY_READ_DATA_DELAY(device));
    }
    return -1;
}

/**
 * Datenbyte an den Kontroller senden
 */
static int floppy_write_data(struct floppy_device* device, uint8_t data)
{
    int i;
    uint8_t msr;

    // Wenn der Controller beschaeftigt ist, wird ein bisschen gewartet und
    // danach nochmal probiert
    for (i = 0; i < FLOPPY_WRITE_DATA_TRIES(device); i++) {
        msr = floppy_read_byte(device, FLOPPY_REG_DSR);

        // Pruefen ob der Kontroller bereit ist, Daten von uns zu akzeptieren
        if ((msr & (FLOPPY_MSR_RQM | FLOPPY_MSR_DIO)) == (FLOPPY_MSR_RQM)) {
            floppy_write_byte(device, FLOPPY_REG_DATA, data);
            return 0;
        }
        cdi_sleep_ms(FLOPPY_READ_DATA_DELAY(device));
    }
    return -1;
}

/**
 * Motorstatus setzen, also ein oder aus
 */
static int floppy_drive_motor_set(struct floppy_device* device, int state)
{
    uint8_t dor;
    uint32_t delay;

    if (device->motor == state) {
        return 0;
    }
    
    // Wert des DOR sichern, weil nur das notwendige Bit ueberschrieben werden
    // soll.
    dor = floppy_read_byte(device, FLOPPY_REG_DOR);
    if (state == 1) {
        // Einschalten
        dor |= FLOPPY_DOR_MOTOR(device);
        delay = FLOPPY_DELAY_SPINUP(device);
    } else {
        // Ausschalten
        dor &= ~FLOPPY_DOR_MOTOR(device);
        delay = FLOPPY_DELAY_SPINDOWN(device);
    }
    floppy_write_byte(device, FLOPPY_REG_DOR, dor);
    
    // Dem Laufwerk Zeit geben, anzulaufen
    cdi_sleep_ms(delay);

    device->motor = state;
    device->current_cylinder = 255;
    return 0;
}

/**
 * Aktives Laufwerk setzen
 */
static int floppy_drive_select(struct floppy_device* device)
{
    uint8_t dor = floppy_read_byte(device, FLOPPY_REG_DOR);
    
    dor &= ~FLOPPY_DOR_DRIVE_MASK;
    dor |= FLOPPY_DOR_DRIVE(device);

    floppy_write_byte(device, FLOPPY_REG_DOR, dor);

    return 0;
}

/**
 * Datenrate fuer das Laufwerk setzen
 */
static int floppy_drive_set_dr(struct floppy_device* device)
{
    // TODO: Wie kriegen wir hier das Optimum raus?
    // Mit 500Kb/s sind wir auf der sicheren Seite
    uint8_t dsr = FLOPPY_DSR_500KBPS;
    floppy_write_byte(device, FLOPPY_REG_DSR, dsr);

    return 0;
}

/**
 * Sense interrupt status
 * Wird benutzt um heraus zu finden, ob gewisse Befehle erfolgreich ausgefuehrt
 * wurden, und es dient of auch dazu, dem Kontroller mitzuteilen, dass der
  * Interrupt empfangen wurde.
 */
static int floppy_drive_int_sense(struct floppy_device* device)
{
    // Befehl senden
    // Byte 0:  FLOPPY_CMD_INT_SENSE
    if (floppy_write_data(device, FLOPPY_CMD_INT_SENSE) != 0) {
        return -1;
    }

    // Ergebnis abholen
    // Byte 0:  Status 0
    // Byte 1:  Zylinder auf dem sich der Kopf gerade befindet
    if ((floppy_read_data(device, &device->current_status) != 0) ||
        (floppy_read_data(device, &device->current_cylinder) != 0))
    {
        return -1;
    }
    return 0;
}

/**
 * Specify
 * Diese Funktion konfiguriert den Kontroller im Bezug auf verschiedene delays.
 * Ist im allgemeinen sehr schlecht dokumentiert. Die Erklaerungen basieren
 * groesstenteils auf http://www.osdev.org/wiki/Floppy_Disk_Controller
 */
static void floppy_drive_specify(struct floppy_device* device)
{
    uint8_t head_unload_time;
    uint8_t head_load_time;
    uint8_t step_rate_time;
    
    // Die head unload time legt fest, wie lange der Kontroller nach einem
    // Lese- oder Schreibvorgang warten soll, bis der den Kopf wieder in den
    // "entladenen" Status gebracht wird. Vermutlich wird diese Aktion nicht
    // ausgefuehrt, wenn dazwischen schon wieder ein Lesevorgang eintrifft,
    // aber das ist nicht sicher. Dieser Wert ist abhaengig von der Datenrate.
    // Laut dem OSdev-Wiki kann hier fuer die bestimmung des optimalen Wertes
    // die folgende Berechnung benutzt werden:
    //      head_unload_time = seconds * data_rate / 8000
    // Als vernuenftiger Wert fuer die Zeit, die gewartet werden soll, wird
    // dort 240ms vorgeschlagen. Wir uebernehmen das hier mal so.
    head_unload_time = 240 * FLOPPY_DATA_RATE(device) / 8000 / 1000;

    // Die head load time ist die Zeit, die der Kontroller warten soll, nachdem
    // er den Kopf zum Lesen oder Schreiben positioniert hat, bis der Kopf
    // bereit ist. Auch dieser Wert haengt wieder von der Datenrate ab.
    // Der optimale Wert kann nach OSdev-Wiki folgendermassen errechnet werden:
    //      head_load_time = seconds * data_rate / 1000
    // Die vorgeschlagene Zeit liegt bei 10 Millisekunden.
    head_load_time = 20 * FLOPPY_DATA_RATE(device) / 1000 / 1000;

    // Mit der step rate time wird die Zeit bestimmt, die der Kontroller warten
    // soll, wenn der den Kopf zwischen den einzelnen Spuren bewegt. Wozu das
    // genau dient konnte ich bisher nirgends finden.
    // Der hier einzustellende Wert ist genau wie die vorderen 2 abhaengig von
    // der Datenrate.
    // Aus dem OSdev-Wiki stammt die folgende Formel fuer die Berechnung:
    //      step_rate_time = 16 - seconds * data_rate / 500000
    // Fuer die Zeit wird 8ms empfohlen.
    step_rate_time = 16 - 8 * FLOPPY_DATA_RATE(device) / 500000 / 1000;
    
    // Befehl und Argumente Senden
    // Byte 0:  FLOPPY_CMD_SPECIFY
    // Byte 1:  Bits 0 - 3: Head unload time
    //          Bits 4 - 7: Step rate time
    // Byte 2:  Bit 0: No DMA; Deaktiviert DMA fuer dieses Geraet
    //          Bit 1 - 7: Head load time
    floppy_write_data(device, FLOPPY_CMD_SPECIFY);
    floppy_write_data(device, (head_unload_time & 0xF) | (step_rate_time <<
        4));
    floppy_write_data(device, (head_load_time << 1) | (~device->controller->
        use_dma));

    // Rueckgabewerte gibt es keine
}

/**
 * Configure
 * Legt ein paar weitere Konfigurationsoptionen fuer den Kontroller fest.
 * Vorallem im Zusammenhang mit der Kommunikation zwischen Software und
 * Kontroller.
 */
/*static void floppy_controller_configure(struct floppy_controller* controller)
{
    // Befehl und Argumente Senden
    // Byte 0:  FLOPPY_CMD_CONFIGURE
    // Byte 1:  Bits 0 - 3: FIFO-Threshold (genaue Beschreibung im Header beim
    //                      Makro)
    //          Bit      4: Deaktiviert die interne Polling-Routine. Was das
    //                      fuer Auswirkungen hat muesste noch geklaert werden.
    //          Bit      5: FIFO deaktivieren
    //          Bit      6: Implizites Seek aktivieren (wird nich ueberall
    //                      unterstuetzt, und wird deshalb nicht benutzt hier)
    //          Bit      7: Nicht Definiert
    // Byte 2:  Laut Manual: "Precompensation start track number"; Muss
    //          irgendetwas zwischen 0x0 und 0xFF sein. Wann man es genau
    //          brauchen koennte ist mir nicht klar.
    //
    // TODO
}*/

/**
 * DMA-Status aendern
 *
 * @param state 1 falls DMA benutzt werden soll, 0 sonst
 */
/*static void floppy_dma_state_set(struct floppy_device* device, int state)
{
    // Nur 0 und 1 sind gueltige Werte fuer das DMA-Feld
    device->controller->use_dma = (state != 0);

    // Jetzt muss das Geraet noch neu Konfiguriert werden
    floppy_drive_specify(device);

    // floppy_drive_configure(device->controller);
}*/

/**
 * Neu kalibrieren
 * Diese Funktion hilft vorallem dabei, Fehler die beim Seek, Read oder auch
 * write auftreten auftreten zu beheben.
 */
static int floppy_drive_recalibrate(struct floppy_device* device)
{
    int i;
    
    // Wenn das neu kalibrieren fehlschlaegt, wird mehrmals probiert.
    for (i = 0; i < FLOPPY_RECAL_TRIES(device); i++) {
        floppy_reset_irqcnt(device);
        
        // Befehl und Attribut senden
        // Byte 0:  FLOPPY_CMD_RECALIBRATE
        // Byte 1:  Bit 0 und 1: Geraetenummer
        floppy_write_data(device, FLOPPY_CMD_RECALIBRATE);
        floppy_write_data(device, device->id);

        // Auf den IRQ warten, wenn der nicht kommt, innerhalb der angebenen
        // Frist, stimmt vermutlich etwas nicht.
        if (floppy_wait_irq(device, FLOPPY_RECAL_TIMEOUT(device)) != 0) {
            // Timeout
            continue;
        }
        
        // Rueckgabewerte existieren nicht.

        // Aktualisiert current_cylinder damit geprueft werden kann, ob
        // erfolgreich neu kalibriert wurde und signalisiert dem FDC, dass der
        // IRQ behandelt wurde.
        floppy_drive_int_sense(device);

        // Nach erfolgreichem neukalibrieren steht der Kopf ueber dem
        // Zylinder 0.
        if (device->current_cylinder == 0) {
            return 0;
        }
    }

    return -1;
}

/**
 * Lese- und Schreibkopf des Diskettenlaufwerks auf einen neuen Zylinder
 * einstellen
 */
static int floppy_drive_seek(struct floppy_device* device, uint8_t cylinder,
    uint8_t head)
{
    uint8_t msr;
    int i;

    // Pruefen ob der Kopf nicht schon richtig steht, denn dann kann etwas Zeit
    // eingespart werden, indem nicht neu positioniert wird.
    if (device->current_cylinder == cylinder) {
        return 0;
    }
    
    // Wenn der seek beendet ist, kommt ein irq an.
    floppy_reset_irqcnt(device);

    // Befehl und Attribute senden
    // Byte 0:  FLOPPY_CMD_SEEK
    // Byte 1:  Bit 0 und 1: Laufwerksnummer
    //          Bit 2: Head
    // Byte 2:
    floppy_write_data(device, FLOPPY_CMD_SEEK);
    floppy_write_data(device, (head << 2) | device->id);
    floppy_write_data(device, cylinder);
    
    // Auf IRQ warten, der kommt, sobald der Kopf positioniert ist.
    if (floppy_wait_irq(device, FLOPPY_SEEK_TIMEOUT(device)) != 0) {
        // Timeout
        return -1;
    }
    
    // Int sense holt die aktuelle Zylindernummer und teilt dem Kontroller mit,
    // dass sein IRQ abgearbeitet wurde. Die Zylindernummer wird benutzt, um
    // festzustellen ob der Seek erfolgreich verlief.
    floppy_drive_int_sense(device);

    // Dem Kopf Zeit geben, sich sauber einzustellen
    cdi_sleep_ms(FLOPPY_SEEK_DELAY(device));
    
    // Warten bis das Laufwerk bereit ist
    for (i = 0; i < 5; i++) { 
        msr = floppy_read_byte(device, FLOPPY_REG_MSR);
        
        if ((msr & (FLOPPY_MSR_DRV_BSY(device) | FLOPPY_MSR_CMD_BSY)) == 0) {
            break;
        }
        cdi_sleep_ms(FLOPPY_SEEK_DELAY(device));
    }
    
    // Wenn das Laufwerk nicht funktioniert muesste jetzt etwas dagegen
    // unternommen werden.
    if (i >= 5) {
        return -1;
    }

    // Pruefen ob der Seek geklappt hat anhand der neuen Zylinderangabge aus
    // int_sense
    if (device->current_cylinder != cylinder) {
        printf("Fehler beim seek: Zylinder nach dem Seek nach %d ist %d\n",
            cylinder, device->current_cylinder);
        return -1;
    }

    // Pruefen ob der Seek erfolgreich abgeschlossen wurde anhand des Seek-End
    // Bits im Statusregister
    if ((device->current_status & FLOPPY_ST0_SEEK_END) != FLOPPY_ST0_SEEK_END)
    {
        return -1;
    }

    return 0;
}

/**
 * Sektor einlesen
 */
static int floppy_drive_sector_read(struct floppy_device* device, uint32_t lba,
    void* buffer)
{
    uint8_t sector;
    uint8_t cylinder;
    uint8_t head;
    uint8_t status = 0;
    uint8_t data;
    struct cdi_dma_handle dma_handle;

    // Adresse in CHS umwandeln weil READ nur CHS als Parameter nimmt
    lba2chs(device, lba, &cylinder, &head, &sector);
    
    // Richtiges Laufwerk auswaehlen
    floppy_drive_select(device);

    // Und bevor irgendetwas gemacht werden kann, stellen wir sicher, dass der
    // Motor laeuft.
    floppy_drive_motor_set(device, 1);

    // Kopf richtig positionieren
    if (floppy_drive_seek(device, cylinder, head) != 0) {
        // Neu kalibrieren und nochmal probiren
        floppy_drive_recalibrate(device);
        if (floppy_drive_seek(device, cylinder, head) != 0) {
           return -1;
        }
    }
    
    // Wenn DMA aktiviert ist, wird es jetzt initialisiert
    if ((device->controller->use_dma != 0) && (cdi_dma_open(&dma_handle,
        FLOPPY_DMA_CHANNEL(device), CDI_DMA_MODE_READ | CDI_DMA_MODE_SINGLE,
        FLOPPY_SECTOR_SIZE(device), buffer) != 0))
    {
        return -1;
    }

    floppy_reset_irqcnt(device);
    // Befehl und Argumente senden
    // Byte 0: FLOPPY_CMD_READ
    // Byte 1: Bit 2:       Kopf (0 oder 1)
    //         Bits 0/1:    Laufwerk
    // Byte 2: Zylinder
    // Byte 3: Kopf
    // Byte 4: Sektornummer (bei Multisektortransfers des ersten Sektors)
    // Byte 5: Sektorgroesse (Nicht in bytes, sondern logarithmisch mit 0 fuer
    //                      128, 1 fuer 256 usw.)
    // Byte 6: Letzte Sektornummer in der aktuellen Spur
    // Byte 7: Gap Length
    // Byte 8: DTL (ignoriert, wenn Sektorgroesse != 0. 0xFF empfohlen)
    floppy_write_data(device, FLOPPY_CMD_READ);
    floppy_write_data(device, (head << 2) | device->id);
    floppy_write_data(device, cylinder);
    floppy_write_data(device, head);
    floppy_write_data(device, sector);
    floppy_write_data(device, FLOPPY_SECTOR_SIZE_CODE(device));
    floppy_write_data(device, FLOPPY_SECTORS_PER_TRACK(device));
    floppy_write_data(device, FLOPPY_GAP_LENGTH(device));
    floppy_write_data(device, 0xFF);

    if (device->controller->use_dma == 0) {
        puts("PIOread");
        // Mit PIO
        size_t bytes_done = 0;
        
        // Bei PIO kommt nach jedem gelesenen Byte ein Interrupt
        while (bytes_done < FLOPPY_SECTOR_SIZE(device)) {
            if (floppy_wait_irq(device, FLOPPY_READ_TIMEOUT(device)) != 0) {
                return -1;
            }
            // IRQ bestaetigen
            floppy_drive_int_sense(device);
            floppy_reset_irqcnt(device);
            floppy_read_data(device, (uint8_t*) buffer + bytes_done++);
        }
    }

    // Sobald der Vorgang beendet ist, kommt wieder ein IRQ
    if (floppy_wait_irq(device, FLOPPY_READ_TIMEOUT(device)) != 0) {
        // Timeout
        if (device->controller->use_dma != 0) {
            cdi_dma_close(&dma_handle);
        }
        return -1;
    }
    
    // Die Rueckgabewerte kommen folgendermassen an:
    // Byte 0: Status 0
    // Byte 1: Status 1
    // Byte 2: Status 2
    // Byte 3: Zylinder
    // Byte 4: Kopf
    // Byte 5: Sektornummer
    // Byte 6: Sektorgroesse (siehe oben)
    floppy_read_data(device, &status);
    floppy_read_data(device, &data);
    floppy_read_data(device, &data);
    floppy_read_data(device, &data);
    floppy_read_data(device, &data);
    floppy_read_data(device, &data);
    floppy_read_data(device, &data);

    // Pruefen ob der Status in Ordnung ist
    if ((status & FLOPPY_ST0_IC_MASK) != FLOPPY_ST0_IC_NORMAL) {
        if (device->controller->use_dma != 0) {
            cdi_dma_close(&dma_handle);
        }

        return -1;
    }
    
    // Bei DMA koennen die Daten jetzt aus dem Buffer eingelesen werden, und
    // das DMA-Handle muss geschlossen werden.
    if ((device->controller->use_dma != 0) && (cdi_dma_read(&dma_handle) != 0))
    {
        cdi_dma_close(&dma_handle);
        return -1;
    } else if (device->controller->use_dma != 0) {
        cdi_dma_close(&dma_handle);
    }

    return 0;
}

/**
 * Sektor schreiben
 */
static int floppy_drive_sector_write(struct floppy_device* device, uint32_t lba,
    void* buffer)
{
    uint8_t sector;
    uint8_t cylinder;
    uint8_t head;
    uint8_t status = 0;
    uint8_t data;
    struct cdi_dma_handle dma_handle;

    // Adresse in CHS-Format umwandeln, weil die Funktionen nur das als
    // Parameter nehmen.
    lba2chs(device, lba, &cylinder, &head, &sector);
    
    // Richtiges Laufwerk auswaehlen
    floppy_drive_select(device);

    // Und bevor irgendetwas gemacht werden kann, stellen wir sicher, dass der
    // Motor laeuft.
    floppy_drive_motor_set(device, 1);

    // Kopf richtig positionieren
    if (floppy_drive_seek(device, cylinder, head) != 0) {
        // Neu kalibrieren und nochmal probiren
        floppy_drive_recalibrate(device);
        if (floppy_drive_seek(device, cylinder, head) != 0) {
           return -1;
        }
    }

    // DMA vorbereiten
    if (cdi_dma_open(&dma_handle, FLOPPY_DMA_CHANNEL(device),
        CDI_DMA_MODE_WRITE | CDI_DMA_MODE_SINGLE, FLOPPY_SECTOR_SIZE(device),
        buffer) != 0)
    {
        return -1;
    }

    // DMA-Buffer fuellen
    if (cdi_dma_write(&dma_handle) != 0) {
        cdi_dma_close(&dma_handle);
        return -1;
    }

   
    floppy_reset_irqcnt(device);
    // Befehl und Argumente senden
    // Byte 0: FLOPPY_CMD_WRITE
    // Byte 1: Bit 2:       Kopf (0 oder 1)
    //         Bits 0/1:    Laufwerk
    // Byte 2: Zylinder
    // Byte 3: Kopf
    // Byte 4: Sektornummer (bei Multisektortransfers des ersten Sektors)
    // Byte 5: Sektorgroesse (Nicht in bytes, sondern logarithmisch mit 0 fuer
    //                      128, 1 fuer 256 usw.)
    // Byte 6: Letzte Sektornummer in der aktuellen Spur
    // Byte 7: Gap Length
    // Byte 8: DTL (ignoriert, wenn Sektorgroesse != 0. 0xFF empfohlen)
    floppy_write_data(device, FLOPPY_CMD_WRITE);
    floppy_write_data(device, (head << 2) | device->id);
    floppy_write_data(device, cylinder);
    floppy_write_data(device, head);
    floppy_write_data(device, sector);
    floppy_write_data(device, FLOPPY_SECTOR_SIZE_CODE(device));
    floppy_write_data(device, FLOPPY_SECTORS_PER_TRACK(device));
    floppy_write_data(device, FLOPPY_GAP_LENGTH(device));
    floppy_write_data(device, 0xFF);

    // Wenn der Sektor geschrieben ist, kommt ein IRQ
    if (floppy_wait_irq(device, FLOPPY_READ_TIMEOUT(device)) != 0) {
        cdi_dma_close(&dma_handle);
        return -1;
    }
    
    // Die Rueckgabewerte kommen folgendermassen an:
    // Byte 0: Status 0
    // Byte 1: Status 1
    // Byte 2: Status 2
    // Byte 3: Zylinder
    // Byte 4: Kopf
    // Byte 5: Sektornummer
    // Byte 6: Sektorgroesse (siehe oben)
    floppy_read_data(device, &status);
    floppy_read_data(device, &data);
    floppy_read_data(device, &data);
    floppy_read_data(device, &data);
    floppy_read_data(device, &data);
    floppy_read_data(device, &data);
    floppy_read_data(device, &data);
    
    // Pruefen ob der Status in Ordnung ist
    if ((status & FLOPPY_ST0_IC_MASK) != FLOPPY_ST0_IC_NORMAL) {
        cdi_dma_close(&dma_handle);
        return -1;
    }

    cdi_dma_close(&dma_handle);
    return 0;
}

/**
 * Pruefen ob ein Laufwerk existiert
 *
 * @return 1 wenn das Laufwerk existiert, 0 sonst
 */
int floppy_device_probe(struct floppy_device* device)
{
    // Read floppy presence from CMOS
    uint8_t t = cdi_cmos_read(0x10);

    // Each nibble contains information about the device type
    if(device->id == 0) {
        return (t >> 4);
    }
    else if(device->id == 1){
        return (t & 0xF);
    }

    return 0;
}

/**
 * Kontroller resetten
 */
static int floppy_reset_controller(struct floppy_device* device)
{
    uint8_t dor = floppy_read_byte(device, FLOPPY_REG_DOR);
    
    floppy_reset_irqcnt(device);

    // No-Reset Bit loeschen
    dor &= ~FLOPPY_DOR_NRST;
    floppy_write_byte(device, FLOPPY_REG_DOR, dor);
    
    // Wir wollen Interrupts bei Datentransfers
    dor |= FLOPPY_DOR_DMAGATE;

    // No-Reset Bit wieder setzen
    dor |= FLOPPY_DOR_NRST;
    floppy_write_byte(device, FLOPPY_REG_DOR, dor);

    return floppy_wait_irq(device, FLOPPY_RESET_TIMEOUT(device));
}

/**
 * Kontroller(-Struktur) initialisieren
 */
int floppy_init_controller(struct floppy_controller* controller)
{
    int i;
    int j;
    // FIXME siehe unten
    struct floppy_device device;

    // Standardports eintragen
    controller->ports[FLOPPY_REG_DOR] = 0x3F2;
    controller->ports[FLOPPY_REG_MSR] = 0x3F4;
    controller->ports[FLOPPY_REG_DSR] = 0x3F4;
    controller->ports[FLOPPY_REG_DATA] = 0x3F5;
    
    // Ports reservieren
    for (i = 0; i < __FLOPPY_REG_MAX; i++) {
        if (controller->ports[i] != 0) {
            if (cdi_ioports_alloc(controller->ports[i], 1) != 0) {
                // Port wurde nicht erfolgreich alloziert. Alle bisher
                // alloziertern Ports muessen freigegeben werden
                for (j = 0; j < i; j++) {
                    cdi_ioports_free(controller->ports[j], 1);
                }

                // Danach wird abgebrochen
                return -1;
            }
        }
    }
    

    // Kontroller resetten
    // FIXME: Hier muss ein device gebastelt werden, weil floppy_read_byte und
    // floppy_write_byte ein device als Parameter brauchen. Wahnsinnig schoen
    // sieht das ja nicht aus...
    device.controller = controller;
    floppy_reset_controller(&device);
    return 0;
}

/**
 * Diskettenlaufwerk vorbereiten
 */
void floppy_init_device(struct cdi_device* device)
{
    struct floppy_device* dev = (struct floppy_device*) device;
    
    // Laufwerk auf aktiv setzen
    floppy_drive_select(dev);
    
    // Datenrate einstellen
    floppy_drive_set_dr(dev);

    // TODO: Hier waere ein Kommentar doch nicht zuviel verlangt, oder? ;-)
    floppy_drive_int_sense(dev);
    floppy_drive_specify(dev);
    floppy_drive_recalibrate(dev);

    dev->dev.block_size = FLOPPY_SECTOR_SIZE(dev);
    dev->dev.block_count = FLOPPY_SECTOR_COUNT(dev);

    cdi_storage_device_init(&dev->dev);
}

/**
 * Diskettenlaufwerk deinitialisieren
 */
void floppy_remove_device(struct cdi_device* device)
{
}

/**
 * Sektoren einlesen
 */
int floppy_read_blocks(struct cdi_storage_device* dev, uint64_t block,
    uint64_t count, void* buffer)
{
    struct floppy_device* device = (struct floppy_device*) dev;
    int result = 0;
    uint32_t i;
    int j;
    
    // Sektoren einlesen
    for (i = 0; (i < count) && (result == 0); i++) {
        result = 1;

        // Wenn das schief geht, wird mehrmals probiert
        for (j = 0; (j < 5) && (result != 0); j++) {
            result = floppy_drive_sector_read(device, (uint32_t) block + i,
                (char*) buffer + i * dev->block_size);
        }
    }

    return result;
}

/**
 * Sektoren schreiben
 */
int floppy_write_blocks(struct cdi_storage_device* dev, uint64_t block,
    uint64_t count, void* buffer)
{
    struct floppy_device* device = (struct floppy_device*) dev;
    int result = 0;
    uint32_t i;
    int j;
    
    // Sektoren einlesen
    for (i = 0; (i < count) && (result == 0); i++) {
        result = 1;

        // Wenn das schief geht, wird mehrmals probiert
        for (j = 0; (j < 5) && (result != 0); j++) {
            result = floppy_drive_sector_write(device, (uint32_t) block + i,
                (char*) buffer + i * dev->block_size);
        }
    }

    return result;
}


/**
 * IRQ-Handler fuer den Kontroller; Erhoeht nur den IRQ-Zaehler in der
 * Kontrollerstruktur
 */
void floppy_handle_interrupt(struct cdi_device* device)
{
    struct floppy_device* dev = (struct floppy_device*) device;
    dev->controller->irq_count++;
}

/**
 * IRQ-Zaehler fuer Kontroller auf 0 setzen
 */
static void floppy_reset_irqcnt(struct floppy_device* device)
{
    device->controller->irq_count = 0;
}

/**
 * Warten bis der IRQ-Zaehler != 0 wird, das heisst bis ein IRQ aufgetreten ist
 */
static int floppy_wait_irq(struct floppy_device* device, uint32_t timeout)
{
    // TODO: Timeout
    while (device->controller->irq_count == 0);
    return 0;
}

