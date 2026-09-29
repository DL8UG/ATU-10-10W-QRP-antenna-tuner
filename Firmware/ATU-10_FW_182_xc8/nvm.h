// Relay setting in the data EEPROM (FW 1.8)
#ifndef NVM_H
#define NVM_H

void nvm_save(void);      // store the relay setting, only changed bytes
char nvm_restore(void);   // 1: setting restored, 0: EEPROM empty or damaged

#endif
