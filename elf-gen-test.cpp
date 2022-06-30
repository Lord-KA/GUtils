#include "elf-gen.h"

static const uint8_t program[] = {
    0x50,
    0x49, 0xb9, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x50,
    0x48, 0xc7, 0xc0, 0x3c, 0x00, 0x00, 0x00, // mov rax, 60
    0x48, 0xc7, 0xc7, 0x00, 0x00, 0x00, 0x00, // mov rdi, 0
    0x0f, 0x05                                // syscall
};


int main()
{
    FILE *out = fopen("test.out", "w");
    assert(out != NULL);

    Elf64_Ehdr ehdr = elf_hdr_setup();
    Elf64_Phdr phdr = progr_hdr_setup(sizeof(program));
    fwrite(&ehdr, sizeof(ehdr), 1, out);
    fwrite(&phdr, sizeof(phdr), 1, out);
    const size_t offset_len = CODE_OFFSET_ - sizeof(ehdr) - sizeof(phdr);
    uint8_t offset[offset_len] = {};
    fwrite(offset,  sizeof(uint8_t), offset_len,  out);
    fwrite(program, sizeof(uint8_t), sizeof(program), out);
    fclose(out);
}
