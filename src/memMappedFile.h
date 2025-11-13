#ifndef MMFILE_H_INCLUDED
#define MMFILE_H_INCLUDED

#include <sys/mman.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>

template <typename FDTYPE> class mmFileManager {
    private:
        const int mmFlag = MAP_FILE|MAP_PRIVATE; // Copy on write
        const int mmProto = PROT_READ|PROT_WRITE;
        const int mmFileMode = S_IRUSR|S_IWUSR|S_IRGRP|S_IROTH;
        const int mmFileOpenMode = O_RDWR|O_CREAT;
        int mmfd = 0;
        int mmerrno = 0;
        size_t mmlen = 0;
        FDTYPE* mmFileAddr = nullptr;
        std::string mmFileName;
        bool mmValid = false;

    public:
        void mmFileClose() {
            if (close(mmfd) != 0) {
                mmerrno = errno;
                fprintf(stderr, "File close error\ncause: %d, %s\n", mmerrno, strerror(mmerrno));
            }
        }

        mmFileManager(size_t length, std::string fileName) {
            if (length == 0) {
                return;
            }
            mmlen = length*sizeof(FDTYPE);

            FILE* tempf = nullptr;
            tempf = fopen(fileName.c_str(), "wb");
            char zero = 0;
            for (ssize_t tempctr = 0; tempctr < mmlen; tempctr++) {
                fwrite(&zero, 1, 1, tempf);
            }
            fclose(tempf);
            
            mmfd = open(fileName.c_str(), mmFileOpenMode);
            mmerrno = errno;
            if (mmfd > 0) {
                fchmod(mmfd, mmFileMode);
                printf("Opend file: name: %s, File descriptor: %d\n", fileName.c_str(), mmfd);
            } else {
                fprintf(stderr, "File name: %s, File oppen error\ncause: %d, %s\n", fileName.c_str(), mmerrno, strerror(mmerrno));
                mmValid = false;
                return;
            }
            mmFileAddr = (FDTYPE*)mmap(0, mmlen, mmProto, mmFlag, mmfd, 0);
            if (mmFileAddr == (FDTYPE*)MAP_FAILED) {
                mmerrno = errno;
                fprintf(stderr, "memory map error: %d (%s)\n", mmerrno, strerror(mmerrno));
                mmFileClose();
                mmValid = false;
                return;
            }
            mmValid = true;
        }

        ~mmFileManager() {
            if (mmValid && mmFileAddr &&
               (munmap(mmFileAddr, mmlen) != 0)) {
                mmerrno = errno;
                fprintf(stderr, "memory unmap error: %d (%s)\n", mmerrno, strerror(mmerrno));
            }
            if (mmfd > 0) {
                mmFileClose();
            }
        }
        FDTYPE* const getArray() {
            return mmFileAddr;
        }
};

#endif