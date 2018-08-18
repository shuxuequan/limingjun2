#include<string.h>
#include<stdio.h>
#include "flagConfig.h"
measureFlag _measureFlag;

void init__measureFlag(){
	memset(&_measureFlag,0x00,sizeof(measureFlag));
	_measureFlag.rate=1;
}

