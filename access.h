#ifndef ACCESS_H
#define ACCESS_H

extern int access_mustmake(int must_make_mtime, struct file *dep);
extern void access_commands(struct file *file, int *must_make);
extern int access_cache(struct file *file);
extern void access_open(struct file *file);
extern void access_child(struct child *child);
extern void access_close(struct file *file);

#endif
