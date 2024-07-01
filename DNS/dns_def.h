//
// ACTUAL DEFINITIONS USED IN `dns_hosts_spoof.c`
//

//
// ** MAIN REPO: https://github.com/junjiemars/c/blob/master
//

// DNS struct definitions
// https://github.com/junjiemars/c/blob/master/src/net/dns/c.c

// countof() function definition
// https://github.com/junjiemars/c/blob/master/src/net/_net_.h

//
// RFC 1035: https://www.ietf.org/rfc/rfc1035.txt
//

#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>

#ifdef countof
#undef countof
#endif
#define countof(a) (sizeof(a) / sizeof(*(a)))

#define DNS_PTR_NAME 0xc0

/* length */
#define DNS_QNAME_MAX_LEN 255
#define DNS_LABEL_MAX_LEN 64
#define DNS_UDP_MAX_LEN 512

/* class */
#define DNS_CLASS_MAP(XX)
XX(1, IN) // Internet
XX(2, CS)
XX(3, CH)
XX(4, HS)

/* type */
#define DNS_TYPE_MAP(XX)
XX(0x01, A)  // A record
XX(0x02, NS) // NS RECORD
XX(0x03, MD)
XX(0x04, MF)
XX(0x05, CNAME)
XX(0x06, SOA)
XX(0x07, MB)
XX(0x08, MG)
XX(0x09, MR)
XX(0x0a, NULL)
XX(0x0b, WKS)
XX(0x0c, PTR)
XX(0x0d, HINFO)
XX(0x0e, MX)
XX(0x0f, TXT)

#define tr_dns_type_str(n)                 \
  ((size_t)(n - 1) < countof(dns_type_str) \
       ? dns_type_str[(size_t)(n - 1)]     \
       : dns_type_str[0])

#define tr_dns_class_str(n)                 \
  ((size_t)(n - 1) < countof(dns_class_str) \
       ? dns_class_str[(size_t)(n - 1)]     \
       : dns_class_str[0])

#define tr_dns_flags_str(a, n) \
  ((size_t)(n) < countof(a) ? a[(size_t)(n)] : ";; invalid")

static char *dns_type_str[] = {
#define XX(n, s) #s,
    DNS_TYPE_MAP(XX)
#undef XX
};

static char *dns_class_str[] = {
#define XX(n, s) #s,
    DNS_CLASS_MAP(XX)
#undef XX
};

enum dns_type
{
#define XX(n, s) DNS_TYPE_##s = n,
  DNS_TYPE_MAP(XX)
#undef XX
};

/* header section */
#pragma pack(push, 1)
typedef struct s_dns_hs
{
  uint16_t id; // 16 bit as defined by uint16_t, thus the colon field (: 16) is not needed
  struct flags
  {
#if (NM_HAVE_LITTLE_ENDIAN)
    uint16_t rcode : 4;  // 4 bits
    uint16_t z : 3;      // 3 bits
    uint16_t ra : 1;     // 1 bit
    uint16_t rd : 1;     // 1 bit
    uint16_t tc : 1;     // 1 bit
    uint16_t aa : 1;     // 1 bit
    uint16_t opcode : 4; // 4 bits
    uint16_t qr : 1;     // 1  bit
#else
    uint16_t qr : 1;
    uint16_t opcode : 4;
    uint16_t aa : 1;
    uint16_t tc : 1;
    uint16_t rd : 1;
    uint16_t ra : 1;
    uint16_t z : 3;
    uint16_t rcode : 4;
#endif /* NM_HAVE_LITTLE_ENDIAN */
  } flags;
  uint16_t qdcount; // 16 bit
  uint16_t ancount; // 16 bit
  uint16_t nscount; // 16 bit
  uint16_t arcount; // 16 bit
} s_dns_hs;
#pragma pack(pop)

/* question section */
#pragma pack(push, 1)
typedef struct s_dns_qs
{
  uint32_t type : 16;  // 16 bit
  uint32_t class : 16; // 16 bit
} s_dns_qs;
#pragma pack(pop)

void parse_label(const uint8_t *buf, const uint8_t *offset,
                 uint8_t *name, size_t *name_len)
{
  const uint8_t *p;
  uint8_t *d, len;
  uint16_t ptr;
  size_t n;

  p = offset;
  d = name;
  n = 0;

  while (n < DNS_LABEL_MAX_LEN)
  {
    ptr = *(const uint16_t *)p;
    if (DNS_PTR_NAME == dns_ptr_type(ptr))
    {
      p = buf + dns_ptr_offset(ptr);
      continue;
    }

    if (0 == *p)
    {
      *--d = 0;
      ++n;
      break;
    }

    len = *p++;
    memcpy(&d[0], &p[0], len);
    d += len;
    *d++ = (uint8_t)'.';
    n += 1 + len;
    p += len;
  }

  *name_len = n;
}
