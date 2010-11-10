/*
 * Copyright (c) 1982, 1986, 1990, 1993
 *	The Regents of the University of California.  All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. All advertising materials mentioning features or use of this software
 *    must display the following acknowledgement:
 *	This product includes software developed by the University of
 *	California, Berkeley and its contributors.
 * 4. Neither the name of the University nor the names of its contributors
 *    may be used to endorse or promote products derived from this software
 *    without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE REGENTS AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE REGENTS OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 *
 *	@(#)in.h	8.3 (Berkeley) 1/3/94
 */

#ifndef _NETINET_ICMP6_H
#define _NETINET_ICMP6_H

#include <unistd.h>
#include <sys/types.h>

#include <netinet/in6.h>

// Payload maximum length
#define ICMPV6_PLD_MAXLEN       1232

struct icmp6_hdr
{
    uint8_t         icmp6_type;
    uint8_t         icmp6_code;
    uint16_t        icmp6_cksum;
    union
    {
        uint32_t    icmp6_un_data32[1];
        uint16_t    icmp6_un_data16[2];
        uint8_t     icmp6_un_data8[4];
    } icmp6_dataun;
} PACKED;

#define icmp6_data32    icmp6_dataun.icmp6_un_data32
#define icmp6_data16    icmp6_dataun.icmp6_un_data16
#define icmp6_data8     icmp6_dataun.icmp6_un_data8
#define icmp6_pptr      icmp6_data32[0]
#define icmp6_mtu       icmp6_data32[0]
#define icmp6_id        icmp6_data16[0]
#define icmp6_seq       icmp6_data16[0]
#define icmp6_maxdelay  icmp6_data16[0]

#define ICMP6_DST_UNREACH       1
#define ICMP6_PACKET_TOO_BIG    2
#define ICMP6_TIME_EXCEEDED     3
#define ICMP6_PARAM_PROB        4

#define ICMP6_ECHO_REQUEST      128
#define ICMP6_ECHO_REPLY        129
#define ICMP6_MEMBERSHIP_QUERY  130
#define MLD6_LISTENER_QUERY     130
#define ICMP6_MEMBERSHIP_REPORT 131
#define MLD6_LISTENER_REPORT    131
#define ICMP6_MEMBERSHIP_REDUCTION  132
#define MLD6_LISTENER_DONE          132

#define ND_ROUTER_SOLICIT       133
#define ND_ROUTER_ADVERT        134
#define ND_NEIGHBOR_SOLICIT     135
#define ND_NEIGHBOR_ADVERT      136
#define ND_REDIRECT             137

#define ICMP6_ROUTER_RENUMBERING    138

#define ICMP6_WRUREQUEST        139
#define ICMP6_WRUREPLY          140
#define ICMP6_FQDN_QUERY        139
#define ICMP6_FQDN_REPLY        140
#define ICMP6_NI_QUERY          139
#define ICMP6_NI_REPLY          140

#define MLD6_MTRACE_RESP        200    /* mtrace response(to sender) */
#define MLD6_MTRACE            201    /* mtrace messages */

#define ICMP6_HADISCOV_REQUEST        202    /* XXX To be defined */
#define ICMP6_HADISCOV_REPLY        203    /* XXX To be defined */
  
#define ICMP6_MAXTYPE            203

#define ICMP6_DST_UNREACH_NOROUTE    0    /* no route to destination */
#define ICMP6_DST_UNREACH_ADMIN         1    /* administratively prohibited */
#define ICMP6_DST_UNREACH_NOTNEIGHBOR    2    /* not a neighbor(obsolete) */
#define ICMP6_DST_UNREACH_BEYONDSCOPE    2    /* beyond scope of source address */
#define ICMP6_DST_UNREACH_ADDR        3    /* address unreachable */
#define ICMP6_DST_UNREACH_NOPORT    4    /* port unreachable */

#define ICMP6_TIME_EXCEED_TRANSIT     0    /* ttl==0 in transit */
#define ICMP6_TIME_EXCEED_REASSEMBLY    1    /* ttl==0 in reass */

#define ICMP6_PARAMPROB_HEADER          0    /* erroneous header field */
#define ICMP6_PARAMPROB_NEXTHEADER    1    /* unrecognized next header */
#define ICMP6_PARAMPROB_OPTION        2    /* unrecognized option */

#define ICMP6_INFOMSG_MASK        0x80    /* all informational messages */

#define ICMP6_NI_SUBJ_IPV6    0    /* Query Subject is an IPv6 address */
#define ICMP6_NI_SUBJ_FQDN    1    /* Query Subject is a Domain name */
#define ICMP6_NI_SUBJ_IPV4    2    /* Query Subject is an IPv4 address */

#define ICMP6_NI_SUCCESS    0    /* node information successful reply */
#define ICMP6_NI_REFUSED    1    /* node information request is refused */
#define ICMP6_NI_UNKNOWN    2    /* unknown Qtype */

#define ICMP6_ROUTER_RENUMBERING_COMMAND  0    /* rr command */
#define ICMP6_ROUTER_RENUMBERING_RESULT   1    /* rr result */
#define ICMP6_ROUTER_RENUMBERING_SEQNUM_RESET   255    /* rr seq num reset */

/* Used in kernel only */
#define ND_REDIRECT_ONLINK    0    /* redirect to an on-link node */
#define ND_REDIRECT_ROUTER    1    /* redirect to a better router */

/*
 * Multicast Listener Discovery
 */
struct mld6_hdr {
    struct icmp6_hdr    mld6_hdr;
    struct in6_addr        mld6_addr; /* multicast address */
} __attribute__((__packed__));

#define mld6_type    mld6_hdr.icmp6_type
#define mld6_code    mld6_hdr.icmp6_code
#define mld6_cksum    mld6_hdr.icmp6_cksum
#define mld6_maxdelay    mld6_hdr.icmp6_data16[0]
#define mld6_reserved    mld6_hdr.icmp6_data16[1]

/*
 * Neighbor Discovery
 */

struct nd_router_solicit {    /* router solicitation */
    struct icmp6_hdr     nd_rs_hdr;
    /* could be followed by options */
} __attribute__((__packed__));

#define nd_rs_type    nd_rs_hdr.icmp6_type
#define nd_rs_code    nd_rs_hdr.icmp6_code
#define nd_rs_cksum    nd_rs_hdr.icmp6_cksum
#define nd_rs_reserved    nd_rs_hdr.icmp6_data32[0]

struct nd_router_advert {    /* router advertisement */
    struct icmp6_hdr    nd_ra_hdr;
    uint32_t        nd_ra_reachable;    /* reachable time */
    uint32_t        nd_ra_retransmit;    /* retransmit timer */
    /* could be followed by options */
} __attribute__((__packed__));

#define nd_ra_type        nd_ra_hdr.icmp6_type
#define nd_ra_code        nd_ra_hdr.icmp6_code
#define nd_ra_cksum        nd_ra_hdr.icmp6_cksum
#define nd_ra_curhoplimit    nd_ra_hdr.icmp6_data8[0]
#define nd_ra_flags_reserved    nd_ra_hdr.icmp6_data8[1]
#define ND_RA_FLAG_MANAGED    0x80
#define ND_RA_FLAG_OTHER    0x40
#define ND_RA_FLAG_HA        0x20

/*
 * Router preference values based on draft-draves-ipngwg-router-selection-01.
 * These are non-standard definitions.
 */
#define ND_RA_FLAG_RTPREF_MASK    0x18 /* 00011000 */

#define ND_RA_FLAG_RTPREF_HIGH    0x08 /* 00001000 */
#define ND_RA_FLAG_RTPREF_MEDIUM    0x00 /* 00000000 */
#define ND_RA_FLAG_RTPREF_LOW    0x18 /* 00011000 */
#define ND_RA_FLAG_RTPREF_RSV    0x10 /* 00010000 */

#define nd_ra_router_lifetime    nd_ra_hdr.icmp6_data16[1]

struct nd_neighbor_solicit {    /* neighbor solicitation */
    struct icmp6_hdr    nd_ns_hdr;
    struct in6_addr        nd_ns_target;    /*target address */
    /* could be followed by options */
} __attribute__((__packed__));

#define nd_ns_type        nd_ns_hdr.icmp6_type
#define nd_ns_code        nd_ns_hdr.icmp6_code
#define nd_ns_cksum        nd_ns_hdr.icmp6_cksum
#define nd_ns_reserved        nd_ns_hdr.icmp6_data32[0]

struct nd_neighbor_advert {    /* neighbor advertisement */
    struct icmp6_hdr    nd_na_hdr;
    struct in6_addr        nd_na_target;    /* target address */
    /* could be followed by options */
} __attribute__((__packed__));

#define nd_na_type        nd_na_hdr.icmp6_type
#define nd_na_code        nd_na_hdr.icmp6_code
#define nd_na_cksum        nd_na_hdr.icmp6_cksum
#define nd_na_flags_reserved    nd_na_hdr.icmp6_data32[0]
#if BYTE_ORDER == BIG_ENDIAN
#define ND_NA_FLAG_ROUTER        0x80000000
#define ND_NA_FLAG_SOLICITED        0x40000000
#define ND_NA_FLAG_OVERRIDE        0x20000000
#else
#if BYTE_ORDER == LITTLE_ENDIAN
#define ND_NA_FLAG_ROUTER        0x80
#define ND_NA_FLAG_SOLICITED        0x40
#define ND_NA_FLAG_OVERRIDE        0x20
#endif
#endif

struct nd_redirect {        /* redirect */
    struct icmp6_hdr    nd_rd_hdr;
    struct in6_addr        nd_rd_target;    /* target address */
    struct in6_addr        nd_rd_dst;    /* destination address */
    /* could be followed by options */
} __attribute__((__packed__));

#define nd_rd_type        nd_rd_hdr.icmp6_type
#define nd_rd_code        nd_rd_hdr.icmp6_code
#define nd_rd_cksum        nd_rd_hdr.icmp6_cksum
#define nd_rd_reserved        nd_rd_hdr.icmp6_data32[0]

struct nd_opt_hdr {        /* Neighbor discovery option header */
    uint8_t    nd_opt_type;
    uint8_t    nd_opt_len;
    /* followed by option specific data*/
} __attribute__((__packed__));

#define ND_OPT_SOURCE_LINKADDR        1
#define ND_OPT_TARGET_LINKADDR        2
#define ND_OPT_PREFIX_INFORMATION    3
#define ND_OPT_REDIRECTED_HEADER    4
#define ND_OPT_MTU            5

#define ND_OPT_ROUTE_INFO        200    /* draft-ietf-ipngwg-router-preference, not officially assigned yet */

struct nd_opt_prefix_info {    /* prefix information */
    uint8_t    nd_opt_pi_type;
    uint8_t    nd_opt_pi_len;
    uint8_t    nd_opt_pi_prefix_len;
    uint8_t    nd_opt_pi_flags_reserved;
    uint32_t    nd_opt_pi_valid_time;
    uint32_t    nd_opt_pi_preferred_time;
    uint32_t    nd_opt_pi_reserved2;
    struct in6_addr    nd_opt_pi_prefix;
} __attribute__((__packed__));

#define ND_OPT_PI_FLAG_ONLINK        0x80
#define ND_OPT_PI_FLAG_AUTO        0x40

struct nd_opt_rd_hdr {        /* redirected header */
    uint8_t    nd_opt_rh_type;
    uint8_t    nd_opt_rh_len;
    uint16_t    nd_opt_rh_reserved1;
    uint32_t    nd_opt_rh_reserved2;
    /* followed by IP header and data */
} __attribute__((__packed__));

struct nd_opt_mtu {        /* MTU option */
    uint8_t    nd_opt_mtu_type;
    uint8_t    nd_opt_mtu_len;
    uint16_t    nd_opt_mtu_reserved;
    uint32_t    nd_opt_mtu_mtu;
} __attribute__((__packed__));

struct nd_opt_route_info {    /* route info */
    uint8_t    nd_opt_rti_type;
    uint8_t    nd_opt_rti_len;
    uint8_t    nd_opt_rti_prefixlen;
    uint8_t    nd_opt_rti_flags;
    uint32_t    nd_opt_rti_lifetime;
    /* followed by prefix */
} __attribute__((__packed__));

/*
 * icmp6 namelookup
 */

struct icmp6_namelookup {
    struct icmp6_hdr     icmp6_nl_hdr;
    uint8_t    icmp6_nl_nonce[8];
    int32_t        icmp6_nl_ttl;
#if 0
    uint8_t    icmp6_nl_len;
    uint8_t    icmp6_nl_name[3];
#endif
    /* could be followed by options */
} __attribute__((__packed__));

/*
 * icmp6 node information
 */
struct icmp6_nodeinfo {
    struct icmp6_hdr icmp6_ni_hdr;
    uint8_t icmp6_ni_nonce[8];
    /* could be followed by reply data */
} __attribute__((__packed__));

#define ni_type        icmp6_ni_hdr.icmp6_type
#define ni_code        icmp6_ni_hdr.icmp6_code
#define ni_cksum    icmp6_ni_hdr.icmp6_cksum
#define ni_qtype    icmp6_ni_hdr.icmp6_data16[0]
#define ni_flags    icmp6_ni_hdr.icmp6_data16[1]

#define NI_QTYPE_NOOP        0 /* NOOP  */
#define NI_QTYPE_SUPTYPES    1 /* Supported Qtypes */
#define NI_QTYPE_FQDN        2 /* FQDN (draft 04) */
#define NI_QTYPE_DNSNAME    2 /* DNS Name */
#define NI_QTYPE_NODEADDR    3 /* Node Addresses */
#define NI_QTYPE_IPV4ADDR    4 /* IPv4 Addresses */

#if BYTE_ORDER == BIG_ENDIAN
#define NI_SUPTYPE_FLAG_COMPRESS    0x1
#define NI_FQDN_FLAG_VALIDTTL        0x1
#elif BYTE_ORDER == LITTLE_ENDIAN
#define NI_SUPTYPE_FLAG_COMPRESS    0x0100
#define NI_FQDN_FLAG_VALIDTTL        0x0100
#endif

#ifdef NAME_LOOKUPS_04
#if BYTE_ORDER == BIG_ENDIAN
#define NI_NODEADDR_FLAG_LINKLOCAL    0x1
#define NI_NODEADDR_FLAG_SITELOCAL    0x2
#define NI_NODEADDR_FLAG_GLOBAL        0x4
#define NI_NODEADDR_FLAG_ALL        0x8
#define NI_NODEADDR_FLAG_TRUNCATE    0x10
#define NI_NODEADDR_FLAG_ANYCAST    0x20 /* just experimental. not in spec */
#elif BYTE_ORDER == LITTLE_ENDIAN
#define NI_NODEADDR_FLAG_LINKLOCAL    0x0100
#define NI_NODEADDR_FLAG_SITELOCAL    0x0200
#define NI_NODEADDR_FLAG_GLOBAL        0x0400
#define NI_NODEADDR_FLAG_ALL        0x0800
#define NI_NODEADDR_FLAG_TRUNCATE    0x1000
#define NI_NODEADDR_FLAG_ANYCAST    0x2000 /* just experimental. not in spec */
#endif
#else  /* draft-ietf-ipngwg-icmp-name-lookups-05 (and later?) */
#if BYTE_ORDER == BIG_ENDIAN
#define NI_NODEADDR_FLAG_TRUNCATE    0x1
#define NI_NODEADDR_FLAG_ALL        0x2
#define NI_NODEADDR_FLAG_COMPAT        0x4
#define NI_NODEADDR_FLAG_LINKLOCAL    0x8
#define NI_NODEADDR_FLAG_SITELOCAL    0x10
#define NI_NODEADDR_FLAG_GLOBAL        0x20
#define NI_NODEADDR_FLAG_ANYCAST    0x40 /* just experimental. not in spec */
#elif BYTE_ORDER == LITTLE_ENDIAN
#define NI_NODEADDR_FLAG_TRUNCATE    0x0100
#define NI_NODEADDR_FLAG_ALL        0x0200
#define NI_NODEADDR_FLAG_COMPAT        0x0400
#define NI_NODEADDR_FLAG_LINKLOCAL    0x0800
#define NI_NODEADDR_FLAG_SITELOCAL    0x1000
#define NI_NODEADDR_FLAG_GLOBAL        0x2000
#define NI_NODEADDR_FLAG_ANYCAST    0x4000 /* just experimental. not in spec */
#endif
#endif

struct ni_reply_fqdn {
    uint32_t ni_fqdn_ttl;
    uint8_t ni_fqdn_namelen;
    uint8_t ni_fqdn_name[3];
} __attribute__((__packed__));

struct icmp6_router_renum {
    struct icmp6_hdr    rr_hdr;
    uint8_t    rr_segnum;
    uint8_t    rr_flags;
    uint16_t    rr_maxdelay;
    uint32_t    rr_reserved;
} __attribute__((__packed__));

#define ICMP6_RR_FLAGS_TEST        0x80
#define ICMP6_RR_FLAGS_REQRESULT    0x40
#define ICMP6_RR_FLAGS_FORCEAPPLY    0x20
#define ICMP6_RR_FLAGS_SPECSITE        0x10
#define ICMP6_RR_FLAGS_PREVDONE        0x08

#define rr_type        rr_hdr.icmp6_type
#define rr_code        rr_hdr.icmp6_code
#define rr_cksum    rr_hdr.icmp6_cksum
#define rr_seqnum     rr_hdr.icmp6_data32[0]

struct rr_pco_match {
    uint8_t    rpm_code;
    uint8_t    rpm_len;
    uint8_t    rpm_ordinal;
    uint8_t    rpm_matchlen;
    uint8_t    rpm_minlen;
    uint8_t    rpm_maxlen;
    uint16_t    rpm_reserved;
    struct    in6_addr    rpm_prefix;
} __attribute__((__packed__));

#define RPM_PCO_ADD        1
#define RPM_PCO_CHANGE        2
#define RPM_PCO_SETGLOBAL    3
#define RPM_PCO_MAX        4

struct rr_pco_use {
    uint8_t    rpu_uselen;
    uint8_t    rpu_keeplen;
    uint8_t    rpu_ramask;
    uint8_t    rpu_raflags;
    uint32_t    rpu_vltime;
    uint32_t    rpu_pltime;
    uint32_t    rpu_flags;
    struct    in6_addr rpu_prefix;
} __attribute__((__packed__));
#define ICMP6_RR_PCOUSE_RAFLAGS_ONLINK    0x80
#define ICMP6_RR_PCOUSE_RAFLAGS_AUTO    0x40

#if BYTE_ORDER == BIG_ENDIAN
#define ICMP6_RR_PCOUSE_FLAGS_DECRVLTIME     0x80000000
#define ICMP6_RR_PCOUSE_FLAGS_DECRPLTIME     0x40000000
#elif BYTE_ORDER == LITTLE_ENDIAN
#define ICMP6_RR_PCOUSE_FLAGS_DECRVLTIME     0x80
#define ICMP6_RR_PCOUSE_FLAGS_DECRPLTIME     0x40
#endif

struct rr_result {
    uint16_t    rrr_flags;
    uint8_t    rrr_ordinal;
    uint8_t    rrr_matchedlen;
    uint32_t    rrr_ifid;
    struct    in6_addr rrr_prefix;
} __attribute__((__packed__));
#if BYTE_ORDER == BIG_ENDIAN
#define ICMP6_RR_RESULT_FLAGS_OOB        0x0002
#define ICMP6_RR_RESULT_FLAGS_FORBIDDEN        0x0001
#elif BYTE_ORDER == LITTLE_ENDIAN
#define ICMP6_RR_RESULT_FLAGS_OOB        0x0200
#define ICMP6_RR_RESULT_FLAGS_FORBIDDEN        0x0100
#endif

struct icmp6_filter {
    uint32_t icmp6_filt[8];
};

#define    ICMP6_FILTER_SETPASSALL(filterp) \
    memset(filterp, 0xff, sizeof(struct icmp6_filter))
#define    ICMP6_FILTER_SETBLOCKALL(filterp) \
    memset(filterp, 0x00, sizeof(struct icmp6_filter))

#define    ICMP6_FILTER_SETPASS(type, filterp) \
    (((filterp)->icmp6_filt[(type) >> 5]) |= (1 << ((type) & 31)))
#define    ICMP6_FILTER_SETBLOCK(type, filterp) \
    (((filterp)->icmp6_filt[(type) >> 5]) &= ~(1 << ((type) & 31)))
#define    ICMP6_FILTER_WILLPASS(type, filterp) \
    ((((filterp)->icmp6_filt[(type) >> 5]) & (1 << ((type) & 31))) != 0)
#define    ICMP6_FILTER_WILLBLOCK(type, filterp) \
    ((((filterp)->icmp6_filt[(type) >> 5]) & (1 << ((type) & 31))) == 0)

struct icmp6errstat {
    uint64_t icp6errs_dst_unreach_noroute;
    uint64_t icp6errs_dst_unreach_admin;
    uint64_t icp6errs_dst_unreach_beyondscope;
    uint64_t icp6errs_dst_unreach_addr;
    uint64_t icp6errs_dst_unreach_noport;
    uint64_t icp6errs_packet_too_big;
    uint64_t icp6errs_time_exceed_transit;
    uint64_t icp6errs_time_exceed_reassembly;
    uint64_t icp6errs_paramprob_header;
    uint64_t icp6errs_paramprob_nextheader;
    uint64_t icp6errs_paramprob_option;
    uint64_t icp6errs_redirect;
    uint64_t icp6errs_unknown;
};

struct icmp6stat {
    uint64_t icp6s_error;
    uint64_t icp6s_canterror;
    uint64_t icp6s_toofreq;
    uint64_t icp6s_outhist[256];
    uint64_t icp6s_badcode;
    uint64_t icp6s_tooshort;
    uint64_t icp6s_checksum;
    uint64_t icp6s_badlen;
    uint64_t icp6s_reflect;
    uint64_t icp6s_inhist[256];    
    uint64_t icp6s_nd_toomanyopt;
    struct icmp6errstat icp6s_outerrhist;
#define icp6s_odst_unreach_noroute \
    icp6s_outerrhist.icp6errs_dst_unreach_noroute
#define icp6s_odst_unreach_admin icp6s_outerrhist.icp6errs_dst_unreach_admin
#define icp6s_odst_unreach_beyondscope \
    icp6s_outerrhist.icp6errs_dst_unreach_beyondscope
#define icp6s_odst_unreach_addr icp6s_outerrhist.icp6errs_dst_unreach_addr
#define icp6s_odst_unreach_noport icp6s_outerrhist.icp6errs_dst_unreach_noport
#define icp6s_opacket_too_big icp6s_outerrhist.icp6errs_packet_too_big
#define icp6s_otime_exceed_transit \
    icp6s_outerrhist.icp6errs_time_exceed_transit
#define icp6s_otime_exceed_reassembly \
    icp6s_outerrhist.icp6errs_time_exceed_reassembly
#define icp6s_oparamprob_header icp6s_outerrhist.icp6errs_paramprob_header
#define icp6s_oparamprob_nextheader \
    icp6s_outerrhist.icp6errs_paramprob_nextheader
#define icp6s_oparamprob_option icp6s_outerrhist.icp6errs_paramprob_option
#define icp6s_oredirect icp6s_outerrhist.icp6errs_redirect
#define icp6s_ounknown icp6s_outerrhist.icp6errs_unknown
    uint64_t icp6s_pmtuchg;
    uint64_t icp6s_nd_badopt;
    uint64_t icp6s_badns;
    uint64_t icp6s_badna;
    uint64_t icp6s_badrs;
    uint64_t icp6s_badra;
    uint64_t icp6s_badredirect;
};

#define ICMPV6CTL_STATS             1
#define ICMPV6CTL_REDIRACCEPT       2
#define ICMPV6CTL_REDIRTIMEOUT      3

#define ICMPV6CTL_ND6_PRUNE         6
#define ICMPV6CTL_ND6_DELAY         8
#define ICMPV6CTL_ND6_UMAXTRIES     9
#define ICMPV6CTL_ND6_MMAXTRIES     10
#define ICMPV6CTL_ND6_USELOOPBACK   11

#define ICMPV6CTL_NODEINFO          13
#define ICMPV6CTL_ERRPPSLIMIT       14
#define ICMPV6CTL_ND6_MAXNUDHINT    15
#define ICMPV6CTL_MTUDISC_HIWAT     16
#define ICMPV6CTL_MTUDISC_LOWAT     17
#define ICMPV6CTL_ND6_DEBUG         18
#define ICMPV6CTL_ND6_DRLIST        19
#define ICMPV6CTL_ND6_PRLIST        20
#define ICMPV6CTL_MAXID             21

#define ICMPV6CTL_NAMES { \
    { 0, 0 }, \
    { 0, 0 }, \
    { "rediraccept", CTLTYPE_INT }, \
    { "redirtimeout", CTLTYPE_INT }, \
    { 0, 0 }, \
    { 0, 0 }, \
    { "nd6_prune", CTLTYPE_INT }, \
    { 0, 0 }, \
    { "nd6_delay", CTLTYPE_INT }, \
    { "nd6_umaxtries", CTLTYPE_INT }, \
    { "nd6_mmaxtries", CTLTYPE_INT }, \
    { "nd6_useloopback", CTLTYPE_INT }, \
    { 0, 0 }, \
    { "nodeinfo", CTLTYPE_INT }, \
    { "errppslimit", CTLTYPE_INT }, \
    { "nd6_maxnudhint", CTLTYPE_INT }, \
    { "mtudisc_hiwat", CTLTYPE_INT }, \
    { "mtudisc_lowat", CTLTYPE_INT }, \
    { "nd6_debug", CTLTYPE_INT }, \
    { 0, 0 }, \
    { 0, 0 }, \
}

#define RTF_PROBEMTU    RTF_PROTO1

#endif
