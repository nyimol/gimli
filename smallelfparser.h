#ifndef _SMALLELFPARSER_H
#define _SMALLELFPARSER_H 1

#include "gimli.h"

class SmallELFParser {
    private :
        ElfW(Ehdr)* m_ELFHeader;
        ElfW(Phdr)* m_ProgramHeaderFirstEntry;
        ElfW(Phdr)* m_DynamicProgramHeader;
        ElfW(Shdr)* m_SectionHeaderFirstEntry;
        ElfW(Shdr)* m_dynsymSectionHeader;
        link_map*   m_linkMapFirstEntry;
        int m_selfFD;
        size_t m_GOTPLT;

        int readELFHeader ();
        int readProgramHeaders ();
        int readSectionHeaders ();
        void closeSelfExeFile ();
        ssize_t readData (void*, off_t, size_t);
        int findDynamicProgramHeader ();
        int findFirstLinkMapEntry ();
        ElfW(Shdr)* findSectionByType (unsigned int);
        ElfW(Shdr)* findSectionByIndex (unsigned int);
        ElfW(Shdr)* findSectionByName (char*);

    public :
        SmallELFParser ();
        ~SmallELFParser ();
        int openFile (const char*);
        size_t numberOfSectionHeaders ();
        size_t numberOfProgramHeaders ();
        int VirtualAddressOfGOTPLTInMainExe ();
        link_map* getFirstLinkMapEntry ();
        ssize_t getSymbolIndex (char *);
        ElfW(Rela)* getRelaForIndex (unsigned int);

};

#endif /* __SMALLELFPARSER_H */
