#ifndef __STT_INC__
#define __STT_INC__

#include "sl_btmesh_api.h"

void status_indicator_on_provisioning();

void status_indicator_on_btmesh_event(sl_btmesh_msg_t *evt);

void status_indicator_on_failed();

void status_indicator_on_success();

void status_indicator_on_provisioning();

#endif // __STT_INC__