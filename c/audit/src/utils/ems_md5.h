
#ifndef EMS_MD5_HEADER_FOR_CRYPT____
#define EMS_MD5_HEADER_FOR_CRYPT____



#ifndef MD5_DIGEST_SIZE
#define MD5_DIGEST_SIZE	16
#endif

#ifndef MD5_BLOCK_SIZE
#define MD5_BLOCK_SIZE 64
#endif

int   ems_md5_stream (FILE *stream, void *resblock);
void *ems_md5_buffer (const char *buffer, size_t len, void *resblock);


#endif
