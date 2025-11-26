#ifndef _COVERTION_H
#define _COVERTION_H

void  vdHex2Asc(const char *hex, char *dsp, int count);
void  vdAsc2Hex (const char *dsp, char *hex, int pairs);
void HextoBCD(char* HexaValue,int lenghtHex,char* BCDvalue);
int CleanTrack2(char* pTrack2Buffer, int pTrack2Length);
int CleanTrack1(char* pTrack2Buffer, int pTrack2Length);
int AdjustTrack2(char* pTrack2, int pTrack2Length);
int Script_Format(const char* pScript, int pScriptLength, unsigned char pTag, char* pBufferOut, int pMaxLength);


#endif /*_COVERTION_H*/
