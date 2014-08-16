#include "smallelfparser.h"

int SmallELFParser::openFile (const char* file) {
    if (m_selfFD >= 0) {
        return m_selfFD;
    }

    int fd = -1;
    VERBOSE ("opening file for read.")
    if (file == NULL) {
        fd = open (PROC_SELF_EXE, O_RDONLY);
    } else {
        fd = open (file, O_RDONLY);
    }

    if (fd < 0) {
        LOG ("ERROR opening file")
    }
    m_selfFD = fd;
    return fd;
}

ElfW(Shdr)* SmallELFParser::findSectionByType (unsigned int type) {
    if (readSectionHeaders () != 0) {
        return NULL;
    }
    int index = 0;
    ElfW(Shdr)* sectionHeader = m_SectionHeaderFirstEntry;
    while (sectionHeader[index].sh_type != type && index < numberOfSectionHeaders ())
        index++;
    if (sectionHeader[index].sh_type == type) {
        //FIXME : we should not return a memory address that is under out control ! the outside world is dangerous !
        //FIXME : you can find lots of this kind of bug in almost every public method ! FIX them too plz :)
        return &sectionHeader[index];
    }
    LOG ("section not found.")
    return NULL;
}

ElfW(Shdr)* SmallELFParser::findSectionByIndex (unsigned int index) {
    if (readSectionHeaders () != 0) {
        return NULL;
    }
    if (index < numberOfSectionHeaders ()) {
        return &m_SectionHeaderFirstEntry[index];
    }
    LOG ("section not found.")
    return NULL;
}

ElfW(Shdr)* SmallELFParser::findSectionByName (char* sectionName) {
    if (readSectionHeaders () != 0) {
        return NULL;
    }

    ElfW(Shdr)* strtabSection = findSectionByIndex (m_ELFHeader->e_shstrndx);
    char* stringContent = (char*) malloc (strtabSection->sh_size);
    readData (stringContent, strtabSection->sh_offset, strtabSection->sh_size);
    for (int index = 0; index < numberOfSectionHeaders (); index++) {
        if (!strcmp (sectionName, &stringContent[m_SectionHeaderFirstEntry[index].sh_name])) {
            free (stringContent);
            return &m_SectionHeaderFirstEntry[index];
        }
    }
    free (stringContent);
    return NULL;
}

ssize_t SmallELFParser::getSymbolIndex (char* symbol) {
    ElfW(Shdr)* dynsymSection;
    ElfW(Shdr)* strtabSection;
    dynsymSection = findSectionByType (SHT_DYNSYM);
    if (!dynsymSection) {
        LOG ("ERROR cannot find dynsym section.")
        return -1;
    }

    strtabSection = findSectionByIndex (dynsymSection->sh_link);
    if (!strtabSection) {
        LOG ("ERROR cannot find strtab section.")
        return -1;
    }

    ElfW(Sym)* sym = (ElfW(Sym)*) malloc(dynsymSection->sh_size);
    char* strtabContent = (char*) malloc(strtabSection->sh_size);

    readData (sym, dynsymSection->sh_offset, dynsymSection->sh_size);
    readData (strtabContent, strtabSection->sh_offset, strtabSection->sh_size);

    for (ssize_t index = 0; index < (dynsymSection->sh_size / sizeof (ElfW(Sym))); index++) {
        if (strcmp (symbol, &strtabContent[sym[index].st_name]) == 0) {
            VERBOSE ("found symbol.")
            free (sym);
            free (strtabContent);
            return index;
        }
    }
    VERBOSE ("Symbol not found.")

    free (sym);
    free (strtabContent);
    return -1;
}

ssize_t SmallELFParser::readData (void* buffer, off_t offset, size_t count) {
    openFile (NULL);

    off_t off = lseek(m_selfFD, offset, SEEK_SET);
    if (off != offset) {
        LOG ("ERROR in lseek.");
        return -1;
    }

    ssize_t readResult = read (m_selfFD, buffer, count);
    if (readResult != count) {
        LOG ("Error in read.")
        return -1;
    }

    return readResult;
}

void SmallELFParser::closeSelfExeFile () {
    if (m_selfFD >= 0) {
        VERBOSE ("closing /proc/self/exe.")
        close (m_selfFD);
        m_selfFD = -1;
    }
}


int SmallELFParser::readELFHeader () {
    if (!m_ELFHeader) {
        VERBOSE ("Start reading elf header.")
        ElfW(Ehdr)* ELFHeader = (ElfW(Ehdr)*) malloc (sizeof (ElfW(Ehdr)));

        if (ELFHeader == NULL) {
            LOG ("OUT OF MEMORY.")
            return -1;
        }

        ssize_t readResult = readData (ELFHeader, 0, sizeof (ElfW(Ehdr)));
        if (readResult < 0) {
            LOG ("ERROR while reading elf header.")
            free (ELFHeader);
            return -1;
        }
        
        m_ELFHeader = ELFHeader;
        VERBOSE ("END reading elf header.")
    }
    return 0;
}

int SmallELFParser::readProgramHeaders () {
    if (!m_ProgramHeaderFirstEntry) {
        VERBOSE ("Start of reading program header.")
        if (readELFHeader () == 0) {
            //FIXME : INTEGER OVERFLOW
            size_t size = m_ELFHeader->e_phnum * sizeof (ElfW(Phdr));
            ElfW(Phdr)* programHeader = (ElfW(Phdr)*) malloc (size);
            if (!programHeader) {
                LOG ("ERRPR out of memory.")
                return -1;
            }
            if (readData (programHeader, m_ELFHeader->e_phoff, size) <= 0) {
                LOG ("ERROR reading program header.")
                return -1;
            }
            m_ProgramHeaderFirstEntry = programHeader;
        }
        VERBOSE ("End of reading program header.")
    }

    return 0;
}

int SmallELFParser::findDynamicProgramHeader () {
    if (!m_DynamicProgramHeader) {
        VERBOSE ("Start finding Dynamic program header.")
        readProgramHeaders ();
        for (int index = 0; index < numberOfProgramHeaders (); index++) {
            if (m_ProgramHeaderFirstEntry[index].p_type == PT_DYNAMIC) {
                VERBOSE ("dynamic program header found.")
                m_DynamicProgramHeader = &m_ProgramHeaderFirstEntry[index];
                return 0;
            }
        }
    }
    // ASSERT not to reach here
    LOG ("cannot find Dynamic Program Header")
    return -1;
}

int SmallELFParser::VirtualAddressOfGOTPLTInMainExe () {
    if (!m_GOTPLT) {
        VERBOSE ("Start finding GOT/PLT virtual address.")
        if (findDynamicProgramHeader () != 0) return -1;
        ElfW(Dyn)* Dyn = (ElfW(Dyn)*)m_DynamicProgramHeader->p_vaddr;
        int index = 0;
        while (Dyn[index].d_tag != DT_PLTGOT) {
            index++;
        }
        VERBOSE ("Found PLTGOT")
        m_GOTPLT = Dyn[index].d_un.d_ptr;
        return 0;
    }
    return -1;
}

int SmallELFParser::readSectionHeaders () {
    if (!m_SectionHeaderFirstEntry) {
        VERBOSE ("Start reading section headers.")
        if (readELFHeader () == 0) {
            //FIXME : INTEGER OVERFLOW ! the section number can be big enough to overflow the size_t
            size_t size = m_ELFHeader->e_shentsize * m_ELFHeader->e_shnum;

            ElfW(Shdr)* sectionHeader = (ElfW(Shdr)*)malloc (size);
            if (!sectionHeader) {
                LOG ("ERROR out of memory.")
                return -1;
            }
            if (readData (sectionHeader, m_ELFHeader->e_shoff, size) <= 0) {
                LOG ("ERROR reading section headers.")
                free (sectionHeader);
                return -1;
            }
            m_SectionHeaderFirstEntry = sectionHeader;
        }
        VERBOSE ("End of reading section headers.")
    }
    return 0;
}

int SmallELFParser::findFirstLinkMapEntry () {
    VERBOSE ("Start finding first link map entry.");
    if (!m_linkMapFirstEntry) {
        if (VirtualAddressOfGOTPLTInMainExe () != 0) return -1;
        
        link_map* linkMapEntry;
        memcpy (&linkMapEntry, (void*)(m_GOTPLT + sizeof(size_t)), sizeof(size_t));
        while (linkMapEntry->l_prev) {
            linkMapEntry = linkMapEntry->l_prev;
        }

        m_linkMapFirstEntry = linkMapEntry;
    }
    return 0;
}

link_map* SmallELFParser::getFirstLinkMapEntry () {
    if (findFirstLinkMapEntry () != 0) {
        LOG ("ERROR getting first link_map entry.");
        return NULL;
    }
    return m_linkMapFirstEntry;
}

size_t SmallELFParser::numberOfSectionHeaders () {
    readELFHeader ();
    return m_ELFHeader->e_shnum;
}

ElfW(Rela)* SmallELFParser::getRelaForIndex (unsigned int index) {
    ElfW(Shdr)* relplt = findSectionByName (".rela.plt");
    ElfW(Rela)* rela = (ElfW(Rela)*)malloc (relplt->sh_size);
    readData (rela, relplt->sh_offset, relplt->sh_size);
    for (int counter = 0; counter < (relplt->sh_size / sizeof (ElfW(Rela))); counter++) {
        if (ELF_R_SYM(rela[counter].r_info) == index) {
            ElfW(Rela)* result = (ElfW(Rela)*)malloc (sizeof (ElfW(Rela)));
            memcpy (result, &rela[counter], sizeof (ElfW(Rela)));
            return result;
        }
    }
    return NULL;
}

SmallELFParser::SmallELFParser () :
m_ELFHeader (0),
m_ProgramHeaderFirstEntry (0),
m_DynamicProgramHeader (0),
m_SectionHeaderFirstEntry (0),
m_GOTPLT (0),
m_linkMapFirstEntry (0),
m_dynsymSectionHeader (0),
m_selfFD (-1) { }

size_t SmallELFParser::numberOfProgramHeaders () {
    readELFHeader ();
    return m_ELFHeader->e_phnum;
}

SmallELFParser::~SmallELFParser () {
    VERBOSE ("In SmallELFParser deconstructor")
    if (m_ELFHeader)
        free (m_ELFHeader);

    if (m_ProgramHeaderFirstEntry)
        free (m_ProgramHeaderFirstEntry);

    if (m_SectionHeaderFirstEntry)
        free (m_SectionHeaderFirstEntry);

    closeSelfExeFile ();
}
