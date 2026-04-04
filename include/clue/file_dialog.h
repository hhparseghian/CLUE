#ifndef CLUE_FILE_DIALOG_H
#define CLUE_FILE_DIALOG_H

#include "dialog.h"

/* File dialog mode */
typedef enum {
    CLUE_FILE_OPEN,
    CLUE_FILE_SAVE,
} ClueFileDialogMode;

/* Named file filter (e.g. "C Source Files", ".c .h") */
typedef struct {
    const char *name;       /* display name e.g. "C Source Files" */
    const char *pattern;    /* space-separated extensions e.g. ".c .h" */
} ClueFileFilter;

/* File dialog result */
typedef struct {
    char  path[1024];   /* full path of selected file */
    bool  ok;           /* true if a file was selected, false if cancelled */
} ClueFileDialogResult;

/* Show a file open dialog (blocking).
 * start_dir:    initial directory (NULL = current directory)
 * filters:      array of file filters (NULL = no filtering)
 * filter_count: number of filters (0 = no filtering)
 * "All Files" is always appended automatically. */
ClueFileDialogResult clue_file_dialog_open(const char *title,
                                           const char *start_dir,
                                           const ClueFileFilter *filters,
                                           int filter_count);

/* Show a file save dialog (blocking).
 * start_dir:    initial directory (NULL = current directory)
 * filename:     default filename (NULL = empty)
 * filters:      array of file filters (NULL = no filtering)
 * filter_count: number of filters */
ClueFileDialogResult clue_file_dialog_save(const char *title,
                                           const char *start_dir,
                                           const char *filename,
                                           const ClueFileFilter *filters,
                                           int filter_count);

#endif /* CLUE_FILE_DIALOG_H */
