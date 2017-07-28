struct AssetChange;

void release(AssetChange * ac);

void init_hotloader();

void register_manager(AssetManager * am);

void check_hotloader_modifications();

void shutdown_hotloader();
