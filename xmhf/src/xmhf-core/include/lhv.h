typedef struct {
	int left;
	int top;
	int width;
	int height;
	char color;
} console_vc_t;

void console_clear(console_vc_t *vc);
char console_get_char(console_vc_t *vc, int x, int y);
void console_put_char(console_vc_t *vc, int x, int y, char c);
void console_get_vc(console_vc_t *vc, int num);

