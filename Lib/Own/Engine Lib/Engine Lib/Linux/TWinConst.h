// TWinConst.h: the Windows SDK constants the client uses, with their Windows values. Generated
// from the SDK headers (winuser.h, wingdi.h, imm.h, winerror.h...) for the identifiers of the
// sources that the Linux headers do not define. Included by TWinCompat.h.
//////////////////////////////////////////////////////////////////////

#pragma once

#ifndef ANTIALIASED_QUALITY
#define ANTIALIASED_QUALITY                      4
#endif
#ifndef ATTR_CONVERTED
#define ATTR_CONVERTED                           2
#endif
#ifndef ATTR_FIXEDCONVERTED
#define ATTR_FIXEDCONVERTED                      5
#endif
#ifndef ATTR_INPUT
#define ATTR_INPUT                               0
#endif
#ifndef ATTR_INPUT_ERROR
#define ATTR_INPUT_ERROR                         4
#endif
#ifndef ATTR_TARGET_CONVERTED
#define ATTR_TARGET_CONVERTED                    1
#endif
#ifndef ATTR_TARGET_NOTCONVERTED
#define ATTR_TARGET_NOTCONVERTED                 3
#endif
#ifndef BI_RGB
#define BI_RGB                                   0
#endif
#ifndef BROADCAST_QUERY_DENY
#define BROADCAST_QUERY_DENY                     1112363332
#endif
#ifndef CLIP_DEFAULT_PRECIS
#define CLIP_DEFAULT_PRECIS                      0
#endif
#ifndef CLSCTX_INPROC
#define CLSCTX_INPROC                            3
#endif
#ifndef CONTEXT_ALL
#define CONTEXT_ALL                              1048607
#endif
#ifndef CP_ACP
#define CP_ACP                                   0
#endif
#ifndef CREATE_ALWAYS
#define CREATE_ALWAYS                            2
#endif
#ifndef DEFAULT_CHARSET
#define DEFAULT_CHARSET                          1
#endif
#ifndef DIB_RGB_COLORS
#define DIB_RGB_COLORS                           0
#endif
#ifndef DT_CALCRECT
#define DT_CALCRECT                              1024
#endif
#ifndef DT_CENTER
#define DT_CENTER                                1
#endif
#ifndef DT_RIGHT
#define DT_RIGHT                                 2
#endif
#ifndef DT_VCENTER
#define DT_VCENTER                               4
#endif
#ifndef ENABLE_PROCESSED_OUTPUT
#define ENABLE_PROCESSED_OUTPUT                  1
#endif
#ifndef ENABLE_WRAP_AT_EOL_OUTPUT
#define ENABLE_WRAP_AT_EOL_OUTPUT                2
#endif
#ifndef ERROR_ACCESS_DENIED
#define ERROR_ACCESS_DENIED                      5
#endif
#ifndef ERROR_BAD_PATHNAME
#define ERROR_BAD_PATHNAME                       161
#endif
#ifndef ERROR_BUFFER_OVERFLOW
#define ERROR_BUFFER_OVERFLOW                    111
#endif
#ifndef ERROR_CANNOT_MAKE
#define ERROR_CANNOT_MAKE                        82
#endif
#ifndef ERROR_CURRENT_DIRECTORY
#define ERROR_CURRENT_DIRECTORY                  16
#endif
#ifndef ERROR_DISK_FULL
#define ERROR_DISK_FULL                          112
#endif
#ifndef ERROR_FILE_NOT_FOUND
#define ERROR_FILE_NOT_FOUND                     2
#endif
#ifndef ERROR_HANDLE_DISK_FULL
#define ERROR_HANDLE_DISK_FULL                   39
#endif
#ifndef ERROR_HANDLE_EOF
#define ERROR_HANDLE_EOF                         38
#endif
#ifndef ERROR_INVALID_DRIVE
#define ERROR_INVALID_DRIVE                      15
#endif
#ifndef ERROR_INVALID_HANDLE
#define ERROR_INVALID_HANDLE                     6
#endif
#ifndef ERROR_INVALID_NAME
#define ERROR_INVALID_NAME                       123
#endif
#ifndef ERROR_LOCK_VIOLATION
#define ERROR_LOCK_VIOLATION                     33
#endif
#ifndef ERROR_NEGATIVE_SEEK
#define ERROR_NEGATIVE_SEEK                      131
#endif
#ifndef ERROR_PATH_NOT_FOUND
#define ERROR_PATH_NOT_FOUND                     3
#endif
#ifndef ERROR_SEEK
#define ERROR_SEEK                               25
#endif
#ifndef ERROR_SHARING_VIOLATION
#define ERROR_SHARING_VIOLATION                  32
#endif
#ifndef ERROR_SUCCESS
#define ERROR_SUCCESS                            0
#endif
#ifndef ERROR_TOO_MANY_OPEN_FILES
#define ERROR_TOO_MANY_OPEN_FILES                4
#endif
#ifndef ERROR_WRITE_PROTECT
#define ERROR_WRITE_PROTECT                      19
#endif
#ifndef EXCEPTION_ACCESS_VIOLATION
#define EXCEPTION_ACCESS_VIOLATION               0xC0000005u
#endif
#ifndef EXCEPTION_ARRAY_BOUNDS_EXCEEDED
#define EXCEPTION_ARRAY_BOUNDS_EXCEEDED          0xC000008Cu
#endif
#ifndef EXCEPTION_BREAKPOINT
#define EXCEPTION_BREAKPOINT                     0x80000003u
#endif
#ifndef EXCEPTION_CONTINUE_SEARCH
#define EXCEPTION_CONTINUE_SEARCH                0
#endif
#ifndef EXCEPTION_DATATYPE_MISALIGNMENT
#define EXCEPTION_DATATYPE_MISALIGNMENT          0x80000002u
#endif
#ifndef EXCEPTION_EXECUTE_HANDLER
#define EXCEPTION_EXECUTE_HANDLER                1
#endif
#ifndef EXCEPTION_FLT_DENORMAL_OPERAND
#define EXCEPTION_FLT_DENORMAL_OPERAND           0xC000008Du
#endif
#ifndef EXCEPTION_FLT_DIVIDE_BY_ZERO
#define EXCEPTION_FLT_DIVIDE_BY_ZERO             0xC000008Eu
#endif
#ifndef EXCEPTION_FLT_INEXACT_RESULT
#define EXCEPTION_FLT_INEXACT_RESULT             0xC000008Fu
#endif
#ifndef EXCEPTION_FLT_INVALID_OPERATION
#define EXCEPTION_FLT_INVALID_OPERATION          0xC0000090u
#endif
#ifndef EXCEPTION_FLT_OVERFLOW
#define EXCEPTION_FLT_OVERFLOW                   0xC0000091u
#endif
#ifndef EXCEPTION_FLT_STACK_CHECK
#define EXCEPTION_FLT_STACK_CHECK                0xC0000092u
#endif
#ifndef EXCEPTION_FLT_UNDERFLOW
#define EXCEPTION_FLT_UNDERFLOW                  0xC0000093u
#endif
#ifndef EXCEPTION_GUARD_PAGE
#define EXCEPTION_GUARD_PAGE                     0x80000001u
#endif
#ifndef EXCEPTION_ILLEGAL_INSTRUCTION
#define EXCEPTION_ILLEGAL_INSTRUCTION            0xC000001Du
#endif
#ifndef EXCEPTION_INT_DIVIDE_BY_ZERO
#define EXCEPTION_INT_DIVIDE_BY_ZERO             0xC0000094u
#endif
#ifndef EXCEPTION_INT_OVERFLOW
#define EXCEPTION_INT_OVERFLOW                   0xC0000095u
#endif
#ifndef EXCEPTION_INVALID_DISPOSITION
#define EXCEPTION_INVALID_DISPOSITION            0xC0000026u
#endif
#ifndef EXCEPTION_INVALID_HANDLE
#define EXCEPTION_INVALID_HANDLE                 0xC0000008u
#endif
#ifndef EXCEPTION_IN_PAGE_ERROR
#define EXCEPTION_IN_PAGE_ERROR                  0xC0000006u
#endif
#ifndef EXCEPTION_NONCONTINUABLE_EXCEPTION
#define EXCEPTION_NONCONTINUABLE_EXCEPTION       0xC0000025u
#endif
#ifndef EXCEPTION_PRIV_INSTRUCTION
#define EXCEPTION_PRIV_INSTRUCTION               0xC0000096u
#endif
#ifndef EXCEPTION_SINGLE_STEP
#define EXCEPTION_SINGLE_STEP                    0x80000004u
#endif
#ifndef EXCEPTION_STACK_OVERFLOW
#define EXCEPTION_STACK_OVERFLOW                 0xC00000FDu
#endif
#ifndef E_FAIL
#define E_FAIL                                   (-2147467259)
#endif
#ifndef E_NOINTERFACE
#define E_NOINTERFACE                            (-2147467262)
#endif
#ifndef E_POINTER
#define E_POINTER                                (-2147467261)
#endif
#ifndef FF_DONTCARE
#define FF_DONTCARE                              0
#endif
#ifndef FILE_ATTRIBUTE_DIRECTORY
#define FILE_ATTRIBUTE_DIRECTORY                 16
#endif
#ifndef FILE_ATTRIBUTE_NORMAL
#define FILE_ATTRIBUTE_NORMAL                    128
#endif
#ifndef FILE_BEGIN
#define FILE_BEGIN                               0
#endif
#ifndef FILE_CURRENT
#define FILE_CURRENT                             1
#endif
#ifndef FILE_FLAG_NO_BUFFERING
#define FILE_FLAG_NO_BUFFERING                   536870912
#endif
#ifndef FILE_FLAG_RANDOM_ACCESS
#define FILE_FLAG_RANDOM_ACCESS                  268435456
#endif
#ifndef FILE_FLAG_SEQUENTIAL_SCAN
#define FILE_FLAG_SEQUENTIAL_SCAN                134217728
#endif
#ifndef FILE_FLAG_WRITE_THROUGH
#define FILE_FLAG_WRITE_THROUGH                  0x80000000u
#endif
#ifndef FILE_MAP_READ
#define FILE_MAP_READ                            4
#endif
#ifndef FILE_SHARE_READ
#define FILE_SHARE_READ                          1
#endif
#ifndef FILE_SHARE_WRITE
#define FILE_SHARE_WRITE                         2
#endif
#ifndef FOREGROUND_BLUE
#define FOREGROUND_BLUE                          1
#endif
#ifndef FOREGROUND_GREEN
#define FOREGROUND_GREEN                         2
#endif
#ifndef FOREGROUND_RED
#define FOREGROUND_RED                           4
#endif
#ifndef GCLP_HICON
#define GCLP_HICON                               (-14)
#endif
#ifndef GCLP_HICONSM
#define GCLP_HICONSM                             (-34)
#endif
#ifndef GCS_COMPATTR
#define GCS_COMPATTR                             16
#endif
#ifndef GCS_COMPCLAUSE
#define GCS_COMPCLAUSE                           32
#endif
#ifndef GCS_COMPSTR
#define GCS_COMPSTR                              8
#endif
#ifndef GCS_RESULTSTR
#define GCS_RESULTSTR                            2048
#endif
#ifndef GENERIC_READ
#define GENERIC_READ                             0x80000000u
#endif
#ifndef GENERIC_WRITE
#define GENERIC_WRITE                            1073741824
#endif
#ifndef GWLP_WNDPROC
#define GWLP_WNDPROC                             (-4)
#endif
#ifndef GWL_STYLE
#define GWL_STYLE                                (-16)
#endif
#ifndef GW_CHILD
#define GW_CHILD                                 5
#endif
#ifndef ICON_BIG
#define ICON_BIG                                 1
#endif
#ifndef ICON_SMALL
#define ICON_SMALL                               0
#endif
#ifndef IDABORT
#define IDABORT                                  3
#endif
#ifndef IDCANCEL
#define IDCANCEL                                 2
#endif
#ifndef IDIGNORE
#define IDIGNORE                                 5
#endif
#ifndef IDNO
#define IDNO                                     7
#endif
#ifndef IDOK
#define IDOK                                     1
#endif
#ifndef IDRETRY
#define IDRETRY                                  4
#endif
#ifndef IDYES
#define IDYES                                    6
#endif
#ifndef IMAGE_DIRECTORY_ENTRY_EXPORT
#define IMAGE_DIRECTORY_ENTRY_EXPORT             0
#endif
#ifndef IMAGE_DOS_SIGNATURE
#define IMAGE_DOS_SIGNATURE                      23117
#endif
#ifndef IMAGE_FILE_MACHINE_I386
#define IMAGE_FILE_MACHINE_I386                  332
#endif
#ifndef IMAGE_ICON
#define IMAGE_ICON                               1
#endif
#ifndef IMAGE_NT_SIGNATURE
#define IMAGE_NT_SIGNATURE                       17744
#endif
#ifndef IME_CMODE_LANGUAGE
#define IME_CMODE_LANGUAGE                       3
#endif
#ifndef IME_CMODE_NATIVE
#define IME_CMODE_NATIVE                         1
#endif
#ifndef IMN_CHANGECANDIDATE
#define IMN_CHANGECANDIDATE                      3
#endif
#ifndef IMN_CLOSECANDIDATE
#define IMN_CLOSECANDIDATE                       4
#endif
#ifndef IMN_OPENCANDIDATE
#define IMN_OPENCANDIDATE                        5
#endif
#ifndef IMN_SETCONVERSIONMODE
#define IMN_SETCONVERSIONMODE                    6
#endif
#ifndef INFINITE
#define INFINITE                                 0xFFFFFFFFu
#endif
#ifndef INVALID_FILE_ATTRIBUTES
#define INVALID_FILE_ATTRIBUTES                  0xFFFFFFFFu
#endif
#ifndef INVALID_SET_FILE_POINTER
#define INVALID_SET_FILE_POINTER                 0xFFFFFFFFu
#endif
#ifndef KF_ALTDOWN
#define KF_ALTDOWN                               8192
#endif
#ifndef KF_EXTENDED
#define KF_EXTENDED                              256
#endif
#ifndef KF_REPEAT
#define KF_REPEAT                                16384
#endif
#ifndef KF_UP
#define KF_UP                                    32768
#endif
#ifndef LB_ADDSTRING
#define LB_ADDSTRING                             384
#endif
#ifndef MAX_PATH
#define MAX_PATH                                 260
#endif
#ifndef MB_ABORTRETRYIGNORE
#define MB_ABORTRETRYIGNORE                      2
#endif
#ifndef MB_ICONERROR
#define MB_ICONERROR                             16
#endif
#ifndef MB_ICONMASK
#define MB_ICONMASK                              240
#endif
#ifndef MB_ICONWARNING
#define MB_ICONWARNING                           48
#endif
#ifndef MB_OK
#define MB_OK                                    0
#endif
#ifndef MB_OKCANCEL
#define MB_OKCANCEL                              1
#endif
#ifndef MB_RETRYCANCEL
#define MB_RETRYCANCEL                           5
#endif
#ifndef MB_TYPEMASK
#define MB_TYPEMASK                              15
#endif
#ifndef MB_YESNO
#define MB_YESNO                                 4
#endif
#ifndef MB_YESNOCANCEL
#define MB_YESNOCANCEL                           3
#endif
#ifndef MF_BYCOMMAND
#define MF_BYCOMMAND                             0
#endif
#ifndef MK_CONTROL
#define MK_CONTROL                               8
#endif
#ifndef MK_LBUTTON
#define MK_LBUTTON                               1
#endif
#ifndef MK_MBUTTON
#define MK_MBUTTON                               16
#endif
#ifndef MK_RBUTTON
#define MK_RBUTTON                               2
#endif
#ifndef MK_SHIFT
#define MK_SHIFT                                 4
#endif
#ifndef NO_ERROR
#define NO_ERROR                                 0
#endif
#ifndef OPEN_ALWAYS
#define OPEN_ALWAYS                              4
#endif
#ifndef OPEN_EXISTING
#define OPEN_EXISTING                            3
#endif
#ifndef OUT_CHARACTER_PRECIS
#define OUT_CHARACTER_PRECIS                     2
#endif
#ifndef PAGE_READONLY
#define PAGE_READONLY                            2
#endif
#ifndef PROCESS_QUERY_INFORMATION
#define PROCESS_QUERY_INFORMATION                1024
#endif
#ifndef PROCESS_VM_READ
#define PROCESS_VM_READ                          16
#endif
#ifndef RPC_C_AUTHN_LEVEL_CALL
#define RPC_C_AUTHN_LEVEL_CALL                   3
#endif
#ifndef RPC_C_AUTHN_LEVEL_DEFAULT
#define RPC_C_AUTHN_LEVEL_DEFAULT                0
#endif
#ifndef RPC_C_AUTHN_WINNT
#define RPC_C_AUTHN_WINNT                        10
#endif
#ifndef RPC_C_AUTHZ_NONE
#define RPC_C_AUTHZ_NONE                         0
#endif
#ifndef RPC_C_IMP_LEVEL_IMPERSONATE
#define RPC_C_IMP_LEVEL_IMPERSONATE              3
#endif
#ifndef SC_CLOSE
#define SC_CLOSE                                 61536
#endif
#ifndef SC_MANAGER_ALL_ACCESS
#define SC_MANAGER_ALL_ACCESS                    983103
#endif
#ifndef SERVICE_ALL_ACCESS
#define SERVICE_ALL_ACCESS                       983551
#endif
#ifndef SERVICE_AUTO_START
#define SERVICE_AUTO_START                       2
#endif
#ifndef SERVICE_CONTINUE_PENDING
#define SERVICE_CONTINUE_PENDING                 5
#endif
#ifndef SERVICE_CONTROL_CONTINUE
#define SERVICE_CONTROL_CONTINUE                 3
#endif
#ifndef SERVICE_CONTROL_INTERROGATE
#define SERVICE_CONTROL_INTERROGATE              4
#endif
#ifndef SERVICE_DISABLED
#define SERVICE_DISABLED                         4
#endif
#ifndef SERVICE_NO_CHANGE
#define SERVICE_NO_CHANGE                        0xFFFFFFFFu
#endif
#ifndef SERVICE_PAUSED
#define SERVICE_PAUSED                           7
#endif
#ifndef SERVICE_PAUSE_PENDING
#define SERVICE_PAUSE_PENDING                    6
#endif
#ifndef SERVICE_RUNNING
#define SERVICE_RUNNING                          4
#endif
#ifndef SERVICE_START_PENDING
#define SERVICE_START_PENDING                    2
#endif
#ifndef SERVICE_STOPPED
#define SERVICE_STOPPED                          1
#endif
#ifndef SERVICE_STOP_PENDING
#define SERVICE_STOP_PENDING                     3
#endif
#ifndef SM_CXICON
#define SM_CXICON                                11
#endif
#ifndef SM_CXSMICON
#define SM_CXSMICON                              49
#endif
#ifndef SM_CYICON
#define SM_CYICON                                12
#endif
#ifndef SM_CYSMICON
#define SM_CYSMICON                              50
#endif
#ifndef SORT_DEFAULT
#define SORT_DEFAULT                             0
#endif
#ifndef STARTF_USESHOWWINDOW
#define STARTF_USESHOWWINDOW                     1
#endif
#ifndef SUBLANG_DEFAULT
#define SUBLANG_DEFAULT                          1
#endif
#ifndef SW_HIDE
#define SW_HIDE                                  0
#endif
#ifndef SW_MAXIMIZE
#define SW_MAXIMIZE                              3
#endif
#ifndef SW_MINIMIZE
#define SW_MINIMIZE                              6
#endif
#ifndef SW_RESTORE
#define SW_RESTORE                               9
#endif
#ifndef SW_SHOW
#define SW_SHOW                                  5
#endif
#ifndef SW_SHOWMAXIMIZED
#define SW_SHOWMAXIMIZED                         3
#endif
#ifndef SW_SHOWMINIMIZED
#define SW_SHOWMINIMIZED                         2
#endif
#ifndef SW_SHOWNORMAL
#define SW_SHOWNORMAL                            1
#endif
#ifndef S_OK
#define S_OK                                     0
#endif
#ifndef VK_ADD
#define VK_ADD                                   107
#endif
#ifndef VK_APPS
#define VK_APPS                                  93
#endif
#ifndef VK_BACK
#define VK_BACK                                  8
#endif
#ifndef VK_CANCEL
#define VK_CANCEL                                3
#endif
#ifndef VK_CAPITAL
#define VK_CAPITAL                               20
#endif
#ifndef VK_CLEAR
#define VK_CLEAR                                 12
#endif
#ifndef VK_CONTROL
#define VK_CONTROL                               17
#endif
#ifndef VK_DECIMAL
#define VK_DECIMAL                               110
#endif
#ifndef VK_DELETE
#define VK_DELETE                                46
#endif
#ifndef VK_DIVIDE
#define VK_DIVIDE                                111
#endif
#ifndef VK_DOWN
#define VK_DOWN                                  40
#endif
#ifndef VK_END
#define VK_END                                   35
#endif
#ifndef VK_ESCAPE
#define VK_ESCAPE                                27
#endif
#ifndef VK_F1
#define VK_F1                                    112
#endif
#ifndef VK_F10
#define VK_F10                                   121
#endif
#ifndef VK_F11
#define VK_F11                                   122
#endif
#ifndef VK_F12
#define VK_F12                                   123
#endif
#ifndef VK_F13
#define VK_F13                                   124
#endif
#ifndef VK_F2
#define VK_F2                                    113
#endif
#ifndef VK_F3
#define VK_F3                                    114
#endif
#ifndef VK_F4
#define VK_F4                                    115
#endif
#ifndef VK_F5
#define VK_F5                                    116
#endif
#ifndef VK_F6
#define VK_F6                                    117
#endif
#ifndef VK_F7
#define VK_F7                                    118
#endif
#ifndef VK_F8
#define VK_F8                                    119
#endif
#ifndef VK_F9
#define VK_F9                                    120
#endif
#ifndef VK_HANGUL
#define VK_HANGUL                                21
#endif
#ifndef VK_HANJA
#define VK_HANJA                                 25
#endif
#ifndef VK_HOME
#define VK_HOME                                  36
#endif
#ifndef VK_INSERT
#define VK_INSERT                                45
#endif
#ifndef VK_LBUTTON
#define VK_LBUTTON                               1
#endif
#ifndef VK_LCONTROL
#define VK_LCONTROL                              162
#endif
#ifndef VK_LEFT
#define VK_LEFT                                  37
#endif
#ifndef VK_LMENU
#define VK_LMENU                                 164
#endif
#ifndef VK_LSHIFT
#define VK_LSHIFT                                160
#endif
#ifndef VK_LWIN
#define VK_LWIN                                  91
#endif
#ifndef VK_MBUTTON
#define VK_MBUTTON                               4
#endif
#ifndef VK_MENU
#define VK_MENU                                  18
#endif
#ifndef VK_MULTIPLY
#define VK_MULTIPLY                              106
#endif
#ifndef VK_NEXT
#define VK_NEXT                                  34
#endif
#ifndef VK_NUMLOCK
#define VK_NUMLOCK                               144
#endif
#ifndef VK_NUMPAD0
#define VK_NUMPAD0                               96
#endif
#ifndef VK_NUMPAD1
#define VK_NUMPAD1                               97
#endif
#ifndef VK_NUMPAD2
#define VK_NUMPAD2                               98
#endif
#ifndef VK_NUMPAD3
#define VK_NUMPAD3                               99
#endif
#ifndef VK_NUMPAD4
#define VK_NUMPAD4                               100
#endif
#ifndef VK_NUMPAD5
#define VK_NUMPAD5                               101
#endif
#ifndef VK_NUMPAD6
#define VK_NUMPAD6                               102
#endif
#ifndef VK_NUMPAD7
#define VK_NUMPAD7                               103
#endif
#ifndef VK_NUMPAD8
#define VK_NUMPAD8                               104
#endif
#ifndef VK_NUMPAD9
#define VK_NUMPAD9                               105
#endif
#ifndef VK_OEM_1
#define VK_OEM_1                                 186
#endif
#ifndef VK_OEM_102
#define VK_OEM_102                               226
#endif
#ifndef VK_OEM_2
#define VK_OEM_2                                 191
#endif
#ifndef VK_OEM_3
#define VK_OEM_3                                 192
#endif
#ifndef VK_OEM_4
#define VK_OEM_4                                 219
#endif
#ifndef VK_OEM_5
#define VK_OEM_5                                 220
#endif
#ifndef VK_OEM_6
#define VK_OEM_6                                 221
#endif
#ifndef VK_OEM_7
#define VK_OEM_7                                 222
#endif
#ifndef VK_OEM_COMMA
#define VK_OEM_COMMA                             188
#endif
#ifndef VK_OEM_MINUS
#define VK_OEM_MINUS                             189
#endif
#ifndef VK_OEM_PERIOD
#define VK_OEM_PERIOD                            190
#endif
#ifndef VK_OEM_PLUS
#define VK_OEM_PLUS                              187
#endif
#ifndef VK_PAUSE
#define VK_PAUSE                                 19
#endif
#ifndef VK_PRIOR
#define VK_PRIOR                                 33
#endif
#ifndef VK_PROCESSKEY
#define VK_PROCESSKEY                            229
#endif
#ifndef VK_RBUTTON
#define VK_RBUTTON                               2
#endif
#ifndef VK_RCONTROL
#define VK_RCONTROL                              163
#endif
#ifndef VK_RETURN
#define VK_RETURN                                13
#endif
#ifndef VK_RIGHT
#define VK_RIGHT                                 39
#endif
#ifndef VK_RMENU
#define VK_RMENU                                 165
#endif
#ifndef VK_RSHIFT
#define VK_RSHIFT                                161
#endif
#ifndef VK_RWIN
#define VK_RWIN                                  92
#endif
#ifndef VK_SCROLL
#define VK_SCROLL                                145
#endif
#ifndef VK_SEPARATOR
#define VK_SEPARATOR                             108
#endif
#ifndef VK_SHIFT
#define VK_SHIFT                                 16
#endif
#ifndef VK_SNAPSHOT
#define VK_SNAPSHOT                              44
#endif
#ifndef VK_SPACE
#define VK_SPACE                                 32
#endif
#ifndef VK_SUBTRACT
#define VK_SUBTRACT                              109
#endif
#ifndef VK_TAB
#define VK_TAB                                   9
#endif
#ifndef VK_UP
#define VK_UP                                    38
#endif
#ifndef WA_ACTIVE
#define WA_ACTIVE                                1
#endif
#ifndef WA_INACTIVE
#define WA_INACTIVE                              0
#endif
#ifndef WHEEL_DELTA
#define WHEEL_DELTA                              120
#endif
#ifndef WH_KEYBOARD_LL
#define WH_KEYBOARD_LL                           13
#endif
#ifndef WM_COMMAND
#define WM_COMMAND                               273
#endif
#ifndef WM_IME_NOTIFY
#define WM_IME_NOTIFY                            642
#endif
#ifndef WM_INITDIALOG
#define WM_INITDIALOG                            272
#endif
#ifndef WM_INPUTLANGCHANGE
#define WM_INPUTLANGCHANGE                       81
#endif
#ifndef WM_INPUTLANGCHANGEREQUEST
#define WM_INPUTLANGCHANGEREQUEST                80
#endif
#ifndef WM_KEYDOWN
#define WM_KEYDOWN                               256
#endif
#ifndef WM_LBUTTONDOWN
#define WM_LBUTTONDOWN                           513
#endif
#ifndef WM_MBUTTONDOWN
#define WM_MBUTTONDOWN                           519
#endif
#ifndef WM_MOUSEMOVE
#define WM_MOUSEMOVE                             512
#endif
#ifndef WM_NCHITTEST
#define WM_NCHITTEST                             132
#endif
#ifndef WM_NCLBUTTONDOWN
#define WM_NCLBUTTONDOWN                         161
#endif
#ifndef WM_NCRBUTTONDOWN
#define WM_NCRBUTTONDOWN                         164
#endif
#ifndef WM_POWERBROADCAST
#define WM_POWERBROADCAST                        536
#endif
#ifndef WM_QUIT
#define WM_QUIT                                  18
#endif
#ifndef WM_RBUTTONDOWN
#define WM_RBUTTONDOWN                           516
#endif
#ifndef WM_SETICON
#define WM_SETICON                               128
#endif
#ifndef WM_SYSKEYDOWN
#define WM_SYSKEYDOWN                            260
#endif
#ifndef WM_USER
#define WM_USER                                  1024
#endif
#ifndef WS_CHILD
#define WS_CHILD                                 1073741824
#endif
#ifndef WS_CLIPCHILDREN
#define WS_CLIPCHILDREN                          33554432
#endif
