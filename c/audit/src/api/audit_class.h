
#ifndef AUDIT_LEVEL2_MODULES_CLASS_H
#define AUDIT_LEVEL2_MODULES_CLASS_H

#define MODULE_TYPE_IN		1
#define MODULE_TYPE_OUT		!MODULE_TYPE_IN

typedef enum modules_id_s module_id;

enum modules_id_s {
	mod_ctrl = 0,
	mod_out  = 1,
	mod_net  = 2,
	mod_sys  = 3
};

typedef struct _audit_class_s	audit_class;

struct _audit_class_s {
	unsigned int	(*id)();
	unsigned int	(*type)();
	const char *    (*nick)();
	int		(*init)(audit_class *);
	int		(*process)(audit_class *, unsigned int msgid, unsigned char *);
	int		(*uninit)(audit_class *);
	ems_uint	 flg;
	unsigned char 	*ctx;
};


#endif
