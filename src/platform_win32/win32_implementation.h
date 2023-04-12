#ifndef WIN32_IMPLEMENTATION_H
#include <malloc.h>

File_Buffer platform_open_and_read_entire_file(Arena *arena, utf8 *file_path, u64 file_path_size)
{
  File_Buffer file_buffer = {};

  utf8 *file_path_copy = (utf8 *) arena_push_zero(arena, file_path_size);
  if (file_path_copy != NULL)
  {
    copy_memory_block(file_path_copy, file_path, file_path_size);

    utf16 file_path_utf16[512] = {};

    u32 bytes_written = (u32) MultiByteToWideChar(CP_UTF8, MB_PRECOMPOSED,
                                                  (LPCCH) file_path, (int) file_path_size,
                                                  (wchar_t *) file_path_utf16, array_count(file_path_utf16));

    if (bytes_written == file_path_size)
    {
      HANDLE file_handle = CreateFileW((LPCWSTR) file_path_utf16,
                                       GENERIC_READ, FILE_SHARE_READ,
                                       NULL, OPEN_ALWAYS,
                                       FILE_ATTRIBUTE_NORMAL, NULL);

      if (file_handle != INVALID_HANDLE_VALUE)
      {
        LARGE_INTEGER large_file_size;
        if (GetFileSizeEx(file_handle, &large_file_size))
        {
          u64 file_size = large_file_size.QuadPart;
          u8 *file_buffer_data = (u8 *) arena_push(arena, file_size);

          if (file_buffer_data)
          {
            u32 bytes_read = 0;
            if (ReadFile(file_handle, file_buffer_data, (u32) file_size, (LPDWORD) &bytes_read, NULL))
            {
              assert((bytes_read == file_size) || !"Win32 Error: bytes read does not match expected");

              file_buffer.data           = file_buffer_data;
              file_buffer.file_path.str  = file_path_copy;
              file_buffer.file_path.size = file_path_size;
              file_buffer.size           = file_size;

              CloseHandle(file_handle);
            }
            else
            {
              assert(!"Win32 Error: could not read file");
            }
          }
          else
          {
            assert(!"expected arena allocation to succeed");
          }
        }
        else
        {
          assert(!"Win32 Error: could not get file size");
        }
      }
      else
      {
        assert(!"Win32 Error: could not open file");
      }
    }
    else
    {
      assert(!"Win32 Error: could not convert the file path to UTF-16");
    }
  }
  else
  {
    assert(!"expected arena allocation to succeed");
  }

  return(file_buffer);
}

#define WIN32_IMPLEMENTATION_H
#endif
