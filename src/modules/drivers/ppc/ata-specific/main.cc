#include <Log.h>
#include <Module.h>
#include <processor/types.h>
#include <processor/Processor.h>
#include <machine/Device.h>
#include <machine/openfirmware/Device.h>

static void searchNode(Device *pDev)
{
  for (unsigned int i = 0; i < pDev->getNumChildren(); i++)
  {
    Device *pChild = pDev->getChild(i);

    /// \todo also convert to Controllers.
    if (!strcmp(pChild->getSpecificType(), "pci-ide"))
    {
      // We've found a native-pci ATA device.

      // BAR0 is the command register address, BAR1 is the control register address.
      for (int j = 0; j < pChild->addresses().count(); j++)
      {
        if (!strcmp(pChild->addresses()[j]->m_Name, "bar0"))
          pChild->addresses()[j]->m_Name = String("command");
        if (!strcmp(pChild->addresses()[j]->m_Name, "bar1"))
          pChild->addresses()[j]->m_Name = String("control");
      }
    }

    if (!strcmp(pChild->getSpecificType(), "ata"))
    {
      OFDevice dev (pChild->getOFHandle());
      NormalStaticString compatible;
      dev.getProperty("compatible", compatible);

      if (compatible == "keylargo-ata")
      {
        // iBook's keylargo controller.

        // The reg property gives in the first 32-bits the offset into
        // the parent's BAR0 (mac-io) of all our registers.
        uintptr_t reg;
        dev.getProperty("reg", &reg, 4);
        
        for (unsigned int j = 0; j < pChild->getParent()->addresses().count(); j++)
        {
          if (!strcmp(pChild->getParent()->addresses()[j]->m_Name, "bar0"))
          {
            reg += pChild->getParent()->addresses()[j]->m_Address;
            break;
          }
        }

        pChild->addresses().pushBack(new Device::Address(
                                       String("command"),
                                       reg,
                                       0x160,
                                       false, /* Not IO */
                                       0x10)); /* Padding of 16 */
        pChild->addresses().pushBack(new Device::Address(
                                       String("control"),
                                       reg+0x160, /* From linux source. */
                                       0x100,
                                       false, /* Not IO */
                                       0x10)); /* Padding of 16 */
      }
    }

    // Recurse.
    searchNode(pChild);
  }
}


void entry()
{
  Device *pDev = &Device::root();
  searchNode(pDev);
}

void exit()
{
}

MODULE_NAME("ata-specific");
MODULE_ENTRY(&entry);
MODULE_EXIT(&exit);
MODULE_DEPENDS(0);
