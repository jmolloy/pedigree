/*
 * Copyright (c) 2008 James Molloy, James Pritchett, Jörg Pfähler, Matthew Iselin
 *
 * Permission to use, copy, modify, and distribute this software for any
 * purpose with or without fee is hereby granted, provided that the above
 * copyright notice and this permission notice appear in all copies.
 *
 * THE SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL WARRANTIES
 * WITH REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED WARRANTIES OF
 * MERCHANTABILITY AND FITNESS. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR
 * ANY SPECIAL, DIRECT, INDIRECT, OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES
 * WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN AN
 * ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF
 * OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.
 */
#include <compiler.h>
#include "Rtc.h"

#define INITIAL_RTC_HZ 1024
#define BCD_TO_BIN8(x) (((((x) & 0xF0) >> 4) * 10) + ((x) & 0x0F))
#define BIN_TO_BCD8(x) ((((x) / 10) * 16) + ((x) % 10))

struct periodicIrqInfo_t
{
  size_t Hz;
  uint8_t rateBits;
  uint64_t ns[2];
};

periodicIrqInfo_t periodicIrqInfo[] = 
{
  { 256, 0x08, {3906250ULL, 3906250ULL}},
  { 512, 0x07, {1953125ULL, 1953125ULL}},
  {1024, 0x06, { 976562ULL,  976563ULL}},
  {2048, 0x05, { 488281ULL,  488281ULL}},
  {4096, 0x04, { 244140ULL,  244141ULL}},
  {8192, 0x03, { 122070ULL,  122070ULL}},
};

uint8_t daysPerMonth[] = {31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31};

Rtc Rtc::m_Instance;

bool Rtc::registerHandler(TimerHandler *handler)
{
  // TODO
  return false;
}
size_t Rtc::getYear()
{
  return m_Year;
}
uint8_t Rtc::getMonth()
{
  return m_Month;
}
uint8_t Rtc::getDayOfMonth()
{
  return m_DayOfMonth;
}
uint8_t Rtc::getDayOfWeek()
{
  static size_t monthnumbers[] = {0, 3, 3, 6, 1, 4, 6, 2, 5, 0, 3, 5};

  // Calculate day of week
  uint8_t dayOfWeek = m_DayOfMonth % 7;
  dayOfWeek += monthnumbers[m_Month - 1];
  dayOfWeek += ((m_Year % 100) + ((m_Year % 100) / 4)) % 7;
  dayOfWeek -= ((m_Year / 100) % 4 - 3) * 2;
  if (m_Month < 3)dayOfWeek--;
  dayOfWeek %= 7;

  return dayOfWeek;
}
uint8_t Rtc::getHour()
{
  return m_Hour;
}
uint8_t Rtc::getMinute()
{
  return m_Minute;
}
uint8_t Rtc::getSecond()
{
  return m_Second;
}
uint64_t Rtc::getTickCount()
{
  return m_TickCount / 1000000ULL;
}

bool Rtc::initialise()
{
  // Allocate the I/O port range
  if (m_IoPort.allocate(0x70, 2) == false)
    return false;

  // Register the irq
  IrqManager &irqManager = IrqManager::instance();
  m_IrqId = irqManager.registerIsaIrqHandler(8, this);
  if (m_IrqId == 0)
    return false;

  // Are the RTC values in the CMOS encoded in BCD (or binary)?
  m_bBCD = (read(0x0B) & 0x04) != 0x04;

  // Read the time and date
  if (m_bBCD == true)
  {
    m_Second = BCD_TO_BIN8(read(0x00));
    m_Minute = BCD_TO_BIN8(read(0x02));
    m_Hour = BCD_TO_BIN8(read(0x04));
    m_DayOfMonth = BCD_TO_BIN8(read(0x07));
    m_Month = BCD_TO_BIN8(read(0x08));
    m_Year = BCD_TO_BIN8(read(0x32)) * 100 + BCD_TO_BIN8(read(0x09));
  }
  else
  {
    m_Second = read(0x00);
    m_Minute = read(0x02);
    m_Hour = read(0x04);
    m_DayOfMonth = read(0x07);
    m_Month = read(0x08);
    m_Year = read(0x32) * 100 + read(0x09);
  }

  // Find the initial rtc rate
  uint8_t rateBits = 0x06;
  for (size_t i = 0;i < 6;i++)
    if (periodicIrqInfo[i].Hz == INITIAL_RTC_HZ)
    {
      m_PeriodicIrqInfoIndex = i;
      rateBits = periodicIrqInfo[i].rateBits;
      break;
    }

  // Set the Rate for the periodic IRQ
  uint8_t tmp = read(0x0A);
  write(0x0A, (tmp & 0xF0) | rateBits);

  // Activate the IRQ
  uint8_t statusb = read(0x0B);
  write(0x0B, statusb | 0x40);

  return true;
}
void Rtc::synchronise()
{
  enableRtcUpdates(false);

  // Write the time and date back
  if (m_bBCD == true)
  {
    write(0x00, BIN_TO_BCD8(m_Second));
    write(0x02, BIN_TO_BCD8(m_Minute));
    write(0x04, BIN_TO_BCD8(m_Hour));
    write(0x07, BIN_TO_BCD8(m_DayOfMonth));
    write(0x08, BIN_TO_BCD8(m_Month));
    write(0x09, BIN_TO_BCD8(m_Year % 100));
    write(0x32, BIN_TO_BCD8(m_Year / 100));
  }
  else
  {
    write(0x00, m_Second);
    write(0x02, m_Minute);
    write(0x04, m_Hour);
    write(0x07, m_DayOfMonth);
    write(0x08, m_Month);
    write(0x09, m_Year % 100);
    write(0x32, m_Year / 100);
  }

  enableRtcUpdates(true);
}
void Rtc::uninitialise()
{
  // Deactivate the IRQ
  uint8_t statusb = read(0x0B);
  write(0x0B, statusb & ~0x40);

  synchronise();

  // Unregister the irq
  IrqManager &irqManager = IrqManager::instance();
  irqManager.unregisterHandler(m_IrqId, this);

  // Free the I/O port range
  m_IoPort.free();
}

Rtc::Rtc()
  : m_IoPort(), m_IrqId(0), m_PeriodicIrqInfoIndex(0), m_bBCD(true), m_Year(0), m_Month(0),
    m_DayOfMonth(0), m_Hour(0), m_Minute(0), m_Second(0), m_Nanosecond(0), m_TickCount(0)
{
}

bool Rtc::irq(irq_id_t number)
{
  static size_t index = 0;
  // Update the Tick Count
  uint64_t delta = periodicIrqInfo[m_PeriodicIrqInfoIndex].ns[index];
  index = (index == 0) ? 1 : 0;
  m_TickCount += delta;

  // Calculate the new time/date
  m_Nanosecond += delta;
  if (UNLIKELY(m_Nanosecond >= 1000000ULL))
  {
    ++m_Second;
    m_Nanosecond -= 1000000ULL;

    if (UNLIKELY(m_Second == 60))
    {
      ++m_Minute;
      m_Second = 0;

      if (UNLIKELY(m_Minute == 60))
      {
        ++m_Hour;
        m_Minute = 0;

        if (UNLIKELY(m_Hour == 24))
        {
          ++m_DayOfMonth;
          m_Hour = 0;

          // Are we in a leap year
          bool isLeap = ((m_Year % 4) == 0) & ((m_Year % 100) != 0) | ((m_Year % 400) == 0);

          if (UNLIKELY(((m_DayOfMonth > daysPerMonth[m_Month - 1]) && ((m_Month != 2) || isLeap == false)) ||
                       (m_DayOfMonth > (daysPerMonth[m_Month - 1] + 1))))
          {
            ++m_Month;
            m_DayOfMonth = 1;

            if (UNLIKELY(m_Month > 12))
            {
              ++m_Year;
              m_Month = 1;
            }
          }
        }
      }
    }
  }

  // Acknowledging the IRQ (within the CMOS)
  read(0x0C);

  // TODO: Call handlers?
  return true;
}

void Rtc::setIndex(uint8_t index)
{
  uint8_t idx = m_IoPort.read8(0);
  m_IoPort.write8((idx & 0x80) | (index & 0x7F), 0);
}
void Rtc::waitForUpdateCompletion(uint8_t index)
{
  if (index <= 9 || index == 50)
  {
    setIndex(0x0A);
    while ((m_IoPort.read8(1) & 0x80) == 0x80);
  }
}
void Rtc::enableRtcUpdates(bool enable)
{
  // Write the index
  setIndex(0x0B);

  // Update the status register
  uint8_t statusA = m_IoPort.read8(1);
  m_IoPort.write8((statusA & 0x80) | (enable ? 0 : (1 << 7)), 1);
}
uint8_t Rtc::read(uint8_t index)
{
  // Wait until the RTC Update is completed
  waitForUpdateCompletion(index);

  // Write the index
  setIndex(index);

  // Read the data at that index
  return m_IoPort.read8(1);
}
void Rtc::write(uint8_t index, uint8_t value)
{
  // Wait until the RTC Update is completed
  waitForUpdateCompletion(index);

  // Write the index
  setIndex(index);

  // Write the data to that index
  m_IoPort.write8(value, 1);
}
