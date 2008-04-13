#ifndef __BONITO64_INTERFACE_H
#define __BONITO64_INTERFACE_H

class Bonito64Interface
{
public:
    Bonito64Interface() {}
    virtual ~Bonito64Interface() {}

    virtual IrqManager* getIrqManager() = 0;
    virtual size_t getNumSerial() = 0;
    virtual size_t getNumVga() = 0;
    virtual SchedulerTimer* getSchedulerTimer() = 0;
    virtual Serial* getSerial(size_t n) = 0;
    virtual Timer* getTimer() = 0;
    virtual Vga* getVga(size_t n) = 0;
    virtual void initialise() = 0;

private:
    Bonito64Interface( const Bonito64Interface& source );
    void operator = ( const Bonito64Interface& source );
};


#endif // __BONITO64_INTERFACE_H
