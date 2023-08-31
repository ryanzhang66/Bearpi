#include <string.h>
#include "rtdTypes.h"
#include "rtdText.h"

uint8_t addRtdText(RtdTextTypeStr *typeStr)
{
    typeStr->status = 0x2;
    typeStr->language[0] = 'e';
    typeStr->language[1] = 'n';

    return sizeof(typeStr->status)+sizeof(typeStr->language);
}

void prepareText(NDEFDataStr *data, RecordPosEnu position, uint8_t *text)
{
    data->ndefPosition = position;
    data->rtdType = RTD_TEXT;
    data->rtdPayload = text;
    data->rtdPayloadlength = strlen((const char *)text);
}
