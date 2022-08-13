/*
 * message.h
 *
 *  Created on: Aug 11, 2022
 */

#ifndef SRC_HDTN_MESSAGE_H_
#define SRC_HDTN_MESSAGE_H_

#define HDTN_MSGTYPE_ILINKUP (0xFC03)     // Link available event from scheduler
#define HDTN_MSGTYPE_ILINKDOWN (0xFC04)    // Link unavailable event from scheduler
#define HDTN_MSGTYPE_ROUTEUPDATE (0xFC07) //Route Update Event from Router process
#define ZMQ_POLL_TIMEOUT 250

struct cbhe_eid_t {
    uint64_t nodeId;
    uint64_t serviceId;
};

struct CommonHdr {
    uint16_t type;
    uint16_t flags;
};

struct RouteUpdateHdr {
    CommonHdr base;
    uint8_t unused3;
    uint8_t unused4;
    cbhe_eid_t nextHopEid;
    cbhe_eid_t finalDestEid;
    uint64_t route[20]; //optimal route
};

struct IreleaseStartHdr {
    CommonHdr base;
    uint8_t unused1;
    uint8_t unused2;
    uint8_t unused3;
    uint8_t unused4;
    cbhe_eid_t finalDestinationEid;   // formerly flow ID
    uint64_t rate;      // bytes / sec
    uint64_t duration;  // msec
    cbhe_eid_t prevHopEid;
    cbhe_eid_t nextHopEid;
    uint64_t time;
};

struct IreleaseStopHdr {
    CommonHdr base;
    uint8_t unused1;
    uint8_t unused2;
    uint8_t unused3;
    uint8_t unused4;
    cbhe_eid_t finalDestinationEid;
    cbhe_eid_t prevHopEid;
    cbhe_eid_t nextHopEid;
    uint64_t time;
    uint64_t contact;
};

#endif /* SRC_HDTN_MESSAGE_H_ */
