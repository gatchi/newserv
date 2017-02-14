#ifdef UNICODE
#define tx wchar_t
#else
#define tx char
#endif

int tx_convert_to_sjis(char*,wchar_t*);
int tx_convert_to_sjis(char*,tx*);
int tx_convert_to_unicode(wchar_t*,char*);
int tx_convert_to_unicode(wchar_t*,tx*);

bool tx_add_language_marker(char*,char);
bool tx_add_language_marker(wchar_t*,wchar_t);
bool tx_remove_language_marker(char*);
bool tx_remove_language_marker(wchar_t*);

int tx_replace_char(char*,char,char);
int tx_replace_char(wchar_t*,wchar_t,wchar_t);

int tx_add_color(char*);
int tx_add_color(wchar_t*);

long checksum(void*,int);
int tx_read_string_data(char*,unsigned long,void*,unsigned long);
int tx_read_string_data(wchar_t*,unsigned long,void*,unsigned long);

