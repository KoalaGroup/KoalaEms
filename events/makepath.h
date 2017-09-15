int
make_path(const char *_argpath,
		   int _mode,
		   int _parent_mode);

int
make_dir(const char *dir,
		  const char *dirpath,
		  mode_t mode,
		  int *created_dir_p);
