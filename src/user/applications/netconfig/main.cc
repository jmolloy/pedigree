#include <arpa/inet.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <iostream>
#include <vector>
#include <string>
#include <sstream>

using namespace std;

static uint32_t g_Options = 0;

template <class T>
const char* convert(T t)
{
	stringstream ss;
	string s;
	ss << t;
	ss >> s;
	return s.c_str();
}

namespace Pedigree
{
	/** An IPv4/IPv6 address */
	class IpAddress
	{
	  public:

		/** The type of an IP address */
		enum IpType
		{
		  IPv4 = 0,
		  IPv6
		};

		/** Constructors */
		IpAddress() :
		  m_Type(IPv4), m_Ipv4(0), m_Ipv6(), m_bSet(false)
		{};
		IpAddress(IpType type) :
		  m_Type(type), m_Ipv4(0), m_Ipv6(), m_bSet(false)
		{};
		IpAddress(uint32_t ipv4) :
		  m_Type(IPv4), m_Ipv4(ipv4), m_Ipv6(), m_bSet(true)
		{};
		IpAddress(uint8_t *ipv6) :
		  m_Type(IPv6), m_Ipv4(0), m_Ipv6(), m_bSet(true)
		{
		  memcpy(m_Ipv6, ipv6, 16);
		};

		/** IP setters - only one type is valid at any one point in time */

		void setIp(uint32_t ipv4)
		{
		  m_Ipv4 = ipv4;
		  memset(m_Ipv6, 0, 16);
		  m_bSet = true;
		  m_Type = IPv4;
		}

		void setIp(uint8_t *ipv6)
		{
		  m_Ipv4 = 0;
		  memcpy(m_Ipv6, ipv6, 16);
		  m_bSet = true;
		  m_Type = IPv6;
		}

		/** IP getters */

		inline uint32_t getIp()
		{
		  return m_Ipv4;
		}

		inline void getIp(uint8_t *ipv6)
		{
		  if(ipv6)
			memcpy(ipv6, m_Ipv6, 16);
		}

		/** Type getter */

		IpType getType()
		{
		  return m_Type;
		}

		/** Operators */

		IpAddress& operator = (IpAddress a)
		{
		  if(a.getType() == IPv6)
		  {
			a.getIp(m_Ipv6);
			m_Ipv4 = 0;
			m_Type = IPv6;
			m_bSet = true;
		  }
		  else
			setIp(a.getIp());
		  return *this;
		}
		
		IpAddress& operator = (uint32_t a)
		{
	      setIp(a);
		  return *this;
		}

		bool operator == (IpAddress a)
		{
		  if(a.getType() == IPv4)
			return (a.getIp() == getIp());
		  else
		  {
			// probably use memcmp for IPv6... too lazy to check at the moment
			return false;
		  }
		}

		bool operator == (uint32_t a)
		{
		  if(getType() == IPv4)
			return (a == getIp());
		  else
			return false;
		}

		string toString()
		{
		  /// \todo IPv6
		  if(m_Type == IPv4)
		  {
			string str;
			str.clear();
			str += convert<uint32_t>(m_Ipv4 & 0xff);
			str += ".";
			str += convert<uint32_t>((m_Ipv4 >> 8) & 0xff);
			str += ".";
			str += convert<uint32_t>((m_Ipv4 >> 16) & 0xff);
			str += ".";
			str += convert<uint32_t>((m_Ipv4 >> 24) & 0xff);
			return str;
		  }
		  return "";
		}

	  private:

		IpType      m_Type;

		uint32_t    m_Ipv4; // the IPv4 address
		uint8_t     m_Ipv6[16]; // the IPv6 address

		bool        m_bSet; // has the IP been set yet?
	};
	
	class NetworkDevice
	{
        public:
            NetworkDevice() : name(""), ipv4(), subnet(), gateway(), dns()
            {};
            virtual ~NetworkDevice()
            {};

		    string name;
		    IpAddress ipv4; /// \todo StationInfo in Pedigree
		    IpAddress subnet;
		    IpAddress gateway;
		    vector<IpAddress> dns;
	};
	
	class Network
	{
		public:
            Network() : m_Devices()
			{
				struct NetworkDevice dev;
				dev.name = "eth0";
				dev.ipv4 = 0x0100007f;
				dev.subnet = 0x000000ff;
				dev.gateway = 0;
				dev.dns.clear();
				m_Devices.push_back(dev);
				dev.name = "eth1";
				dev.ipv4 = inet_addr("10.0.0.2");
				dev.subnet = 0x00ffffff;
				dev.gateway = inet_addr("10.0.0.1");
				dev.dns.push_back(inet_addr("10.0.0.10"));
				dev.dns.push_back(inet_addr("10.0.0.11"));
				dev.dns.push_back(inet_addr("10.0.0.12"));
				dev.dns.push_back(inet_addr("10.0.0.13"));
				m_Devices.push_back(dev);
				dev.dns.clear();
				dev.name = "eth2";
				dev.ipv4 = inet_addr("192.168.1.100");
				dev.subnet = 0x00ffffff;
				dev.gateway = inet_addr("192.168.1.254");
				dev.dns.push_back(inet_addr("192.168.1.254"));
				m_Devices.push_back(dev);
			};
			virtual ~Network()
			{
				m_Devices.clear();
			};
			
			static Network& instance()
			{
				return m_Instance;
			}
			
			vector<NetworkDevice>& getDevices()
			{
				return m_Devices;
			}
		
		private:
			static Network m_Instance;
			
			vector<NetworkDevice> m_Devices;
	};
	
	Network Network::m_Instance;
}

int main(int argc, char* argv[])
{
	// Parse arguments
	for(int arg = 1; arg < argc; arg++)
	{
		// TODO: Write Me :D
	}
	
	// Enumerate network devices
	vector<Pedigree::NetworkDevice> devices = Pedigree::Network::instance().getDevices();
	for(vector<Pedigree::NetworkDevice>::iterator it = devices.begin(); it != devices.end(); ++it)
	{
		Pedigree::NetworkDevice dev = *it;
		cout << "Device '" << dev.name << "':" << endl;
		cout << "\tIPv4: " << dev.ipv4.toString() << ", Subnet Mask: " << dev.subnet.toString() << ", Gateway: " << dev.gateway.toString() << endl;
		cout << "\tDNS servers:";
		for(vector<Pedigree::IpAddress>::iterator dns = dev.dns.begin(); dns != dev.dns.end(); ++dns)
			cout << " " << (*dns).toString();
		cout << endl;
	}
	
	return 0;
}
