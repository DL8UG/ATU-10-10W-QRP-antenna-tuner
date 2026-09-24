#include "mikroc_compat.h"
#include "nvm.h"

// The relays are latching: they keep their setting without power, the
// variables in RAM do not. Storing the setting in the data EEPROM keeps both
// in step after a reset or a battery change, and the tuning survives it.
// The EEPROM macros of XC8 are inline, so this costs no hardware stack level.

extern char ind, cap, SW, byp_ind, byp_cap, byp_SW;
extern __bit Bypass;

#define NVM_MAGIC 0xA5
enum { A_MAGIC, A_IND, A_CAP, A_SW, A_BYP, A_BYP_IND, A_BYP_CAP, A_BYP_SW, A_SUM, NVM_LEN };

static char nvm_sum(const char *b){
   char i, s = 0;
   for(i=0; i<A_SUM; i++) s += b[i];
   return (char)~s;   // an erased EEPROM (all 0xFF) does not pass
}
//
void nvm_save(void){
   char b[NVM_LEN], i;
   b[A_MAGIC] = NVM_MAGIC;
   b[A_IND] = ind;
   b[A_CAP] = cap;
   b[A_SW] = SW;
   b[A_BYP] = Bypass;
   b[A_BYP_IND] = byp_ind;
   b[A_BYP_CAP] = byp_cap;
   b[A_BYP_SW] = byp_SW;
   b[A_SUM] = nvm_sum(b);
   for(i=0; i<NVM_LEN; i++){   // write only what changed (endurance)
      while(NVMCON1bits.WR) continue;
      if(EEPROM_READ(i) != b[i]) EEPROM_WRITE(i, b[i]);
   }
   return;
}
//
char nvm_restore(void){
   char b[NVM_LEN], i;
   for(i=0; i<NVM_LEN; i++) b[i] = EEPROM_READ(i);
   if(b[A_MAGIC] != NVM_MAGIC || b[A_SUM] != nvm_sum(b)) return 0;
   ind = b[A_IND];
   cap = b[A_CAP];
   SW = b[A_SW];
   Bypass = b[A_BYP] ? 1 : 0;
   byp_ind = b[A_BYP_IND];
   byp_cap = b[A_BYP_CAP];
   byp_SW = b[A_BYP_SW];
   return 1;
}
