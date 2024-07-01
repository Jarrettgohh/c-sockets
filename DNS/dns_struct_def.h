// ACTUAL DEFINITIONS USED IN `dns_hosts_spoof.c`

// DNS struct definitions
// https://github.com/junjiemars/c/blob/master/src/net/dns/c.c

// countof() function definition
// https://github.com/junjiemars/c/blob/master/src/net/_net_.h

#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>

#ifdef countof
#undef countof
#endif
#define countof(a) (sizeof(a) / sizeof(*(a)))

/* class */
#define DNS_CLASS_MAP(XX) \
 XX(1, IN)                \
 XX(2, CS)                \
 XX(3, CH)                \
 XX(4, HS)

/* type */
#define DNS_TYPE_MAP(XX) \
 XX(0x01, A)             \
 XX(0x02, NS)            \
 XX(0x03, MD)            \
 XX(0x04, MF)            \
 XX(0x05, CNAME)         \
 XX(0x06, SOA)           \
 XX(0x07, MB)            \
 XX(0x08, MG)            \
 XX(0x09, MR)            \
 XX(0x0a, NULL)          \
 XX(0x0b, WKS)           \
 XX(0x0c, PTR)           \
 XX(0x0d, HINFO)         \
 XX(0x0e, MX)            \
 XX(0x0f, TXT)

#define tr_dns_type_str(n)                \
 ((size_t)(n - 1) < countof(dns_type_str) \
      ? dns_type_str[(size_t)(n - 1)]     \
      : dns_type_str[0])

#define tr_dns_class_str(n)                \
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
 uint16_t id;
 struct flags
 {
#if (NM_HAVE_LITTLE_ENDIAN)
  uint16_t rcode : 4;
  uint16_t z : 3;
  uint16_t ra : 1;
  uint16_t rd : 1;
  uint16_t tc : 1;
  uint16_t aa : 1;
  uint16_t opcode : 4;
  uint16_t qr : 1;
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
 uint16_t qdcount;
 uint16_t ancount;
 uint16_t nscount;
 uint16_t arcount;
} s_dns_hs;
#pragma pack(pop)

/* question section */
#pragma pack(push, 1)
typedef struct s_dns_qs
{
 uint32_t type : 16;
 uint32_t class : 16;
} s_dns_qs;
#pragma pack(pop)