// ========================================================================
// Function Prototypes
// ========================================================================

GlobalConfig_t& getGlobalCfg();
void setMotorCfg();
void setPositionCfg(PositionConfig_t& cfg);
void setFlushCfg(FlushConfig_t& cfg);
void setSDCfg(SDConfig_t& cfg);
// void setTimesCfg(TimesConfig_t& cfg);
bool load_cfg_from_sd(const char* filename);
void export_cfg_to_sd();
void initSD();

// ========================================================================
// Functions
// ========================================================================

GlobalConfig_t& getGlobalCfg() {
    return gbl_cfg;
}

// needs to be call before everything
void init_cfg() {
    initSD();

    // read JSON from sd
    load_cfg_from_sd(CONFIG_FILENAME);

    // set up the config to devices
    setMotorCfg();
    setPositionCfg(gbl_cfg.position_cfg);
    setFlushCfg(gbl_cfg.flush_cfg);
    setSDCfg(gbl_cfg.sd_cfg);
    // setTimesCfg(gbl_cfg.times_cfg);
}



