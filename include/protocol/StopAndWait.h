#ifndef __STOP_AND_WAIT_H__
#define __STOP_AND_WAIT_H__
#include <interfaces/packet.h>

typedef enum ErrorHandler{
	NO_ERROR,
	IO_ERROR,
	NO_LINK,
}ErrorHandler;

ErrorHandler protocol_establishment_routine (ProtocolControlEvent event, Control * c, Status * s);
ErrorHandler protocol_control_routine (BYTE * p, Control * c, Status * s);
ErrorHandler StopAndWait(Control * c, Status * s);

#endif