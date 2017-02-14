#include <windows.h>
#include "version.h"

// This function returns the client flags represented by the client's subversion number
// (in the 9x authentication commands).
WORD GetClientSubversionFlags(BYTE version,BYTE psoVersionID)
{
    switch (psoVersionID)
    {
      case 0x00: // initial check (before 9E recognition) 
        switch (version)
        {
          case VERSION_DC:
          case VERSION_GAMECUBE:
          case VERSION_FUZZIQER:
            return 0;
          case VERSION_PC:
          case VERSION_PATCH:
            return 0;
          case VERSION_BLUEBURST:
            return FLAG_PSOV3_BB;
        }
        break;
      case 0x29: // PSO PC 
        return FLAG_PSOV2_PC;
      case 0x30: // ??? 
      case 0x31: // PSO Ep1&2 USA10, USA11, EU10, JAP10 
      case 0x33: // PSO Ep1&2 EU50HZ 
      case 0x34: // PSO Ep1&2 JAP11 
        return FLAG_PSOV3_10;
      case 0x32: // PSO Ep1&2 USA12, JAP12 
      case 0x35: // PSO Ep1&2 USA12, JAP12 
      case 0x36: // PSO Ep1&2 USA12, JAP12 
      case 0x39: // PSO Ep1&2 USA12, JAP12 
        return FLAG_PSOV3_12;
      case 0x40: // PSO Ep3 trial 
      case 0x41: // PSO Ep3 USA 
      case 0x42: // PSO Ep3 JAP? 
      case 0x43: // PSO Ep3 UK 
        return FLAG_PSOV4;
    }
    return 0;
}
