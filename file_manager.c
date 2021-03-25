#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>
#include <dirent.h>
#include <sys/stat.h>

/* START CONSTANT DEFINITIONS */

/* Definition of file action constant values used for the changelog */
#define ACTION_INSERT_LINE 0
#define ACTION_APPEND_LINE 1
#define ACTION_DELETE_LINE 2
#define ACTION_CREATE_FILE 3
#define   ACTION_READ_FILE 4
#define   ACTION_READ_LINE 5

/* Define max file name size */
#define MAX_FILE_NAME_SIZE 255

/* Define max line content size */
#define MAX_LINE_CONTENT_SIZE 2048

/* Define size for current working directory */
#define MAX_FILE_PATH_SIZE 1000

/* Define default input buffer */
#define DEFAULT_INPUT_BUFFER 255

/* Define name for the temporary file used */
#define TEMP_FILE_NAME "file.tmp"

/* Define name of the changelog foler */
#define CHANGELOG_NAME "changelog"

/* Definition of status codes */
#define SUCCESS 0
#define FAILURE -1

/* END CONSTANT DEFINITIONS */

/*
*   Function: fileExists
*   --------------------
*   Determines whether a file with the given name already exists
*
*   file_name: the name of the file to check
*
*   returns: 1 if the file exists, 0 if it doesn't
*            Allows for semantically logical code. For example:
*               1. if (fileExists(...)) -> true when the function returns 1
*               2. if(!fileExists(...)) -> true when the function returns 0
*/

int fileExists(const char *file_name)
{
    return !access(file_name, F_OK);
}

/*
*   Function: openFile
*   ------------------
*   A wrapper for fopen() that opens a file and exits if fopen() fails.
*
*   file_name: the name of the file to open
*   mode: the I/O mode
*
*   returns: a pointer to the newly-opened file, or NULL if the program
*            fails to open the file.
*/

FILE *openFile(const char *file_name, const char *mode)
{
    FILE *file = fopen(file_name, mode);
    if (file == NULL)
    {
        fprintf(stderr, "\n[Error] Failed to open file '%s': %s.\n", file_name, strerror(errno));
        return NULL;
    }
    return file;
}

/*
*   Function: createFile
*   --------------------
*   Creates a new file with the specified name.
*
*   file_name: the name to give the new file.
*
*   returns: SUCCESS on success and FAILURE on failure.
*/

int createFile(const char *file_name)
{
    FILE *file;

    if (fileExists(file_name))
    {
        fprintf(stderr, "\n[Error] Failed to create file '%s': File aready exists.\n", file_name);
        return FAILURE;
    }

    /* Creates a file if it doesn't exist */
    file = openFile(file_name, "a");

    if (!file)
    {
        fprintf(stderr, "\n[Error] Failed to create file '%s': See above for more information.\n", file_name);
        return FAILURE;
    }

    fclose(file);
    return SUCCESS;
}

/*
*   Function: deleteFile
*   --------------------
*   Deletes an existing file with the specified name
*
*   file_name: the name of the file to delete
*/

int deleteFile(const char *file_name)
{
    if (remove(file_name))
    {
        fprintf(stderr, "\n[Error] Failed to delete file '%s': %s\n", file_name, strerror(errno));
        return FAILURE;
    }

    return SUCCESS;
}

/*
*   Function: wrename
*   -----------------
*   A wrapper for rename() to handle errors.
*
*   returns: FAILURE on failure and SUCCESS on success.
*/

int wrename(const char *old_file_name, const char *new_file_name)
{
    if (rename(old_file_name, new_file_name))
    {
        fprintf(stderr, "\n[Error] Failed to rename file '%s' to '%s': %s\n", old_file_name, new_file_name, strerror(errno));
        return FAILURE;
    }
    return SUCCESS;
}

/*
*   Function: getChangelogFileName
*   ------------------------------
*   Gets the name of the changelog for the given file (the file + .changelog).
*
*   file_name: name of the file to get the changelog name of.
*   changelog_file_name: variable to read the changelog file name into.
*   file_name_size: the size of the file_name array
*/

void getChangelogFileName(const char *file_name, char *changelog_file_name, const int file_name_size)
{
    memcpy(changelog_file_name, file_name, file_name_size);
    strcat(changelog_file_name, ".changelog");
}

/*
*   Function: getNumberOfLinesInFile
*   --------------------------------
*   Counts the number of lines in a specified file.
*
*   file_name: the name of the file to count the lines from.
*
*   returns: the number of lines in the specified file
*/

int getNumberOfLinesInFile(FILE *file)
{
    int line_count = 0;
    int current_character;

    /* Whilst the pointer isn't at the end of the file */
    while (current_character != EOF)
    {
        current_character = fgetc(file);
        if (current_character == '\n')
        {
            line_count++;
        }
    }

    /* Set the file pointer to the start of the file once finished */
    fseek(file, 0, SEEK_SET);

    return line_count;
}

/*
*   Function: validateLineNumber
*   ----------------------------
*   Validates whether a given line number is in a file.
*
*   file: the file stream to read from.
*   line_number: the line number to check.
*
*   returns: SUCCESS for a valid line number, otherwise FAILURE.
*/

int validateLineNumber(FILE *file, const int line_number)
{
    int line_count = getNumberOfLinesInFile(file);
    if (line_number > line_count || line_number < 1)
    {
        fprintf(stderr, "\n[Error] Line %d is out of range. Please enter a valid line number.\n", line_number);
        return FAILURE;
    }
    return SUCCESS;
}

/*
*   Function: getFileContents
*   ------------------------
*   Gets the contents of an existing file.
*
*   file: the file steam to read from.
*
*   returns: the contents of the specified file
*/

char *getFileContents(FILE *file)
{
    long size_of_file;

    /* Set the stream to the end of the file */
    fseek(file, 0, SEEK_END);

    /* Get the current value of the position indicator (in bytes) */
    size_of_file = ftell(file);

    /* Set the stream to the beginning of the file */
    fseek(file, 0, SEEK_SET);

    char *file_contents = malloc(size_of_file);
    fread(file_contents, 1, size_of_file, file);

    return file_contents;
}

/*
*   Function: copyFile
*   ------------------
*   Creates a new file with a specified name and the contents of an existing file
*
*   existing_file_name: the name of the file the contents will be copied from.
*   new_file_name: the name of the new file.
*
*   returns: SUCCESS if the new file was created with the source file contents,
*            FAILURE if an operation fails.
*/

int copyFile(const char *source_file_name, const char *new_file_name)
{

    FILE *source_file;
    FILE *new_file;
    char *file_contents;

    if (fileExists(new_file_name))
    {
        fprintf(stderr, "\n[Error] Failed to copy contents from '%s' to '%s': File '%s' already exists.\n", source_file_name, new_file_name, new_file_name);
        return FAILURE;
    }

    source_file = openFile(source_file_name, "r");

    if (!source_file || createFile(new_file_name))
    {
        fprintf(stderr, "\n[Error] Failed to copy contents from '%s' to '%s': See above for more information.\n", source_file_name, new_file_name);
        return FAILURE;
    }

    file_contents = getFileContents(source_file);

    new_file = openFile(new_file_name, "w");
    if (!new_file)
    { return FAILURE; }

    fprintf(new_file, "%s", file_contents);
    fclose(new_file);

    free(file_contents);

    return SUCCESS;


}

/*
*   Function: appendLineToFile
*   --------------------------
*   Creates a new line of content at the end of the specified file.
*
*   file_name: the name of the file to append content to.
*   content: the content to be appended to the file.
*
*   returns: SUCCESS if the content is appended to the file,
*            FAILURE if an operation fails.
*/

int appendLineToFile(const char *file_name, const char *content)
{
    FILE *file;

    if (!fileExists(file_name))
    {
        fprintf(stderr, "\n[Error] Cannot append to file '%s': %s\n", file_name, strerror(errno));
        return FAILURE;
    }

    file = openFile(file_name, "a");
    if (!file)
    {
        fprintf(stderr, "\n[Error] Cannot append to file '%s': See above for more information.\n", file_name);
        return FAILURE;
     }

    fputs(content, file);
    fputs("\n", file);
    fclose(file);

    return SUCCESS;
}

/*
*   Function: displayFile
*   ---------------------
*   Reads the contents of a file and displays them to the console.
*
*   file_name: the name of the file to read from.
*
*   returns: SUCCESS if the content is displayed,
*            FAILURE if an operation fails.
*/

int displayFile(const char *file_name)
{
    FILE *file;
    char *file_contents;

    file = openFile(file_name, "rb");
    if (!file)
    { return FAILURE; }

    file_contents = getFileContents(file);
    fclose(file);
    printf("Contents of file:\n%s", file_contents);

    free(file_contents);
    return SUCCESS;
}

/*
*   Function: insertLineInFile
*   --------------------------
*   Creates a new line of content at a particular line number in the specified file
*
*   file_name: the name of the file to insert content into.
*   content: the content to be inserted.
*   line_number: the line number to insert the content at.
*
*   returns: SUCCESS if the content is inserted withot error,
*            FAILURE if an operation fails.
*/

int insertLineInFile(const char *file_name, const char *content, const int line_number)
{
    FILE *temp_file;
    FILE *file;
    int current_line = 1;
    int current_character;

    file = openFile(file_name, "r");
    if (!file)
    {
        fprintf(stderr, "\n[Error] Failed to insert line into file '%s': See above for more information.\n", file_name);
        return FAILURE;
    }

    int line_count = getNumberOfLinesInFile(file);
    if (line_number > line_count || line_number < 1)
    {
        fprintf(stderr, "\n[Error] Failed to insert content into '%s' at line %d: Please enter a valid line number.\n", file_name, line_number);
        return FAILURE;
    }

    /* Create temporary file to write data to */
    temp_file = openFile(TEMP_FILE_NAME, "w");
    if (!temp_file)
    { return FAILURE; }

    current_character = getc(file);

    while (current_character != EOF)
    {
        if (current_line != line_number)
        {
            putc(current_character, temp_file);
        }
        else
        {
            if (line_number != 1)
            {
                putc('\n', temp_file);
            }

            /* Write specified content into the temporary file */
            fputs(content, temp_file);
            putc('\n', temp_file);
            current_line++;
        }
        current_character = fgetc(file);
        if (current_character == '\n')
        {
            current_line++;
        }

    }

    fclose(file);
    fclose(temp_file);

    /* Attempt to delete the old file and rename the temporary file to the source file name */
    int not_deleted = deleteFile(file_name);
    if (not_deleted || wrename(TEMP_FILE_NAME, file_name))
    {
        fprintf(stderr, "\n[Error] Failed to insert content at line %d from '%s': See above for more information.", line_number, file_name);
        /* Clean up and delete the temp file for future use if original file delete fails */
        if (not_deleted)
        {
            deleteFile(TEMP_FILE_NAME);
        }
        return FAILURE;
    }

    return SUCCESS;
}

/*
*   Function: showLineFromFile
*   --------------------------
*   Displays the contents of the specified file at a particular line number.
*
*   file_name: the name of the file to display the line contents from.
*   line_number: the line number to read the contents from.
*
*   returns: SUCCESS if the line content is displayed,
*            FAILURE if an operation fails.
*/

int showLineFromFile(const char *file_name, const int line_number)
{
    FILE *file;
    int current_line = 1;
    int current_character;

    file = openFile(file_name, "r");
    if (!file || validateLineNumber(file, line_number))
    {
        fprintf(stderr, "\n[Error] Failed to read contents at line %d of '%s'. See above for more information.\n", line_number, file_name);
        return FAILURE;
    }

    printf("Content at line %d:\n", line_number);
    current_character = getc(file);
    while (current_character != EOF)
    {
        /* Since we're only displaying one line, there's no need to display the new line */
        if (current_line == line_number && current_character != '\n')
        {
            printf("%c", current_character);
        }
        current_character = fgetc(file);
        if (current_character == '\n')
        {
            current_line++;
        }

    }
    fclose(file);
    printf("\n");

    return SUCCESS;
}

/*
*   Function: deleteLineFromFile
*   ----------------------------
*   Deletes a line of content at a particular line number in the specified file.
*
*   file_name: the name of the file to delete the line content from.
*   line_number: the line number to delete content at.
*
*   returns: SUCCESS if the line is deleted,
*            FAILURE if an operation fails.
*/

int deleteLineFromFile(const char *file_name, const int line_number)
{
    FILE *temp_file;
    FILE *file;
    int current_line = 1;
    int current_character;

    file = openFile(file_name, "r");
    /* Create temp file to write data to */
    temp_file = openFile(TEMP_FILE_NAME, "w");

    if (!file || validateLineNumber(file, line_number) || !temp_file)
    {
        fprintf(stderr, "\n[Error] Failed to delete line %d from file '%s': See above for more information.\n", line_number, file_name);
        return FAILURE;
    }

    current_character = getc(file);
    while (current_character != EOF)
    {
        if (current_line != line_number)
        {
            putc(current_character, temp_file);
        }
        current_character = fgetc(file);
        if (current_character == '\n')
        {
            current_line++;
        }

    }

    fclose(file);
    fclose(temp_file);

    /* Attempt to delete the old file and rename the temporary file to the source file name */
    int not_deleted = deleteFile(file_name);
    if (not_deleted || wrename(TEMP_FILE_NAME, file_name))
    {
        fprintf(stderr, "\n[Error] Failed to delete line %d from '%s': See above for more information.", line_number, file_name);
        /* Clean up and delete the temporary file for future use if original file delete fails */
        if (not_deleted)
        {
            deleteFile(TEMP_FILE_NAME);
        }
        return FAILURE;
    }
    return SUCCESS;
}

/*
*   Function: displayNumberOfLinesInFile
*   ------------------------------------
*   Displays the number of lines in a file.
*
*   file_name: the name of the file to count lines from.
*
*   returns: SUCCESS if the number of lines is displayed,
*            FAILURE if an operation fails.
*/

int displayNumberOfLinesInFile(const char *file_name)
{
    int line_count;
    FILE *file;

    file = openFile(file_name, "r");
    if (!file)
    { return FAILURE; }

    line_count = getNumberOfLinesInFile(file);
    printf("Number of lines in '%s': %d\n", file_name, line_count);
    fclose(file);

    return SUCCESS;
}

/*
*   Function: showChangelog
*   -----------------------
*   Displays the sequence of operations performed on a file by this program.
*
*   file_name: the name of the file to show the changelog of.
*   changelog_directory: the name of the changelog directory.
*
*   returns: SUCCESS if the changelog is displayed,
*            FAILURE if an operation fails.
*/

int showChangelog(const char *file_name, const char *changelog_directory)
{
    char *file_contents;
    char changelog_file_path[MAX_FILE_PATH_SIZE];
    char changelog_file_name[MAX_FILE_NAME_SIZE];

    /* Take the file name and convert it to the name of its changelog file */
    getChangelogFileName(file_name, changelog_file_name, sizeof(changelog_file_name));
    sprintf(changelog_file_path, "changelog/%s", changelog_file_name);

    if (displayFile(changelog_file_path))
    {
        fprintf(stderr, "\n[Error] Failed to read changelog for file '%s': %s\n", file_name, strerror(errno));
        return FAILURE;
    }
    return SUCCESS;
}

/*
*   Function: resetChangelog
*   ------------------------
*   Resets the changelog for a specified file.
*
*   file_name: the name of the file that will have its changelog reset
*   changelog_directory: the full path to the changelog directory
*
*   returns: SUCCESS if the changelog is reset,
*            FAILURE if an operation fails.
*/

int resetChangelog(const char *file_name, const char *changelog_directory)
{
    FILE *changelog_file;
    char changelog_file_path[MAX_FILE_PATH_SIZE];
    char changelog_file_name[MAX_FILE_NAME_SIZE];

    /* Take the file name and convert it to the name of its changelog file */
    getChangelogFileName(file_name, changelog_file_name, sizeof(changelog_file_name));
    sprintf(changelog_file_path, "changelog/%s", changelog_file_name);

    if (remove(changelog_file_path))
    {
        fprintf(stderr, "\n[Error] Failed to reset changelog for '%s': %s\n", file_name, strerror(errno));
        return FAILURE;
    }
    return SUCCESS;
}

/*
*   Function: addActionToChangeLog
*   -------------------------
*   Updates the change log for a file by inserting the specified action
*   and number of lines affected
*
*   file_name: the name of the file to update the changelog of.
*   action: the constant number for the action performed.
*   changelog_directory: the full path to the changelog directory
*
*   returns: SUCCESS if the action was added to the file's changelog,
*            FAILURE if an operation fails.
*/

int addActionToChangelog(const char *file_name, const int action, const char *changelog_directory)
{
    FILE *changelog_file;
    FILE *source_file;
    int number_of_lines;
    const char *action_strings[] = { "Inserted line", "Appended line", "Deleted line", "Created file", "Read File", "Read Line" };
    char changelog_string[MAX_LINE_CONTENT_SIZE];
    char changelog_file_name[MAX_FILE_NAME_SIZE];

    /* Take the file name and convert it the name of its changelog file */
    getChangelogFileName(file_name, changelog_file_name, sizeof(changelog_file_name));

    char path_to_changelog_file[MAX_FILE_PATH_SIZE];
    sprintf(path_to_changelog_file, "%s/%s", changelog_directory, changelog_file_name);

    changelog_file = openFile(path_to_changelog_file, "a");
    source_file = openFile(file_name, "r");
    if (!changelog_file || !source_file)
    {
        fprintf(stderr, "\n[Error] Failed to write to changelog for file '%s': See above for more information.\n", file_name);
        return FAILURE;
    }

    number_of_lines = getNumberOfLinesInFile(source_file);
    sprintf(changelog_string, "[%s] Number of lines after action: %d", action_strings[action], number_of_lines);
    fclose(changelog_file);

    if (appendLineToFile(path_to_changelog_file, changelog_string))
    {
        fprintf(stderr, "\n[Error] Failed to write to changelog for file '%s': See above for more information.\n", file_name);
        return FAILURE;
    }

    return SUCCESS;
}

/*
*   Function: deleteFileFromChangelog
*   ---------------------------------
*   Deletes a file's changelog.
*
*   file_name: the name of the file to delete the changelog of.
*   changelog_directory: the full path to the changelog directory.
*
*   returns: SUCCESS if the changelog file is deleted,
*            FAILURE if an operation fails.
*/

int deleteFileFromChangelog(const char *file_name, const char *changelog_directory)
{
    char changelog_file_path[MAX_FILE_PATH_SIZE];
    char changelog_file_name[MAX_FILE_NAME_SIZE];

    /* Take the file name and convert it the name of its changelog file */
    getChangelogFileName(file_name, changelog_file_name, sizeof(changelog_file_name));
    sprintf(changelog_file_path, "%s/%s", changelog_directory, changelog_file_name);

    if (deleteFile(changelog_file_path))
    {
        fprintf(stderr, "\n[Error] Failed to delete the changelog for '%s': %s\n", file_name, strerror(errno));
        return FAILURE;
    }
    return SUCCESS;
}


/*
*   Function: getInput
*   ------------------
*   Receives user input and writes it to the specified variable.
*
*   msg: the message shown to the user before receiving input.
*   input_var: the variable to write the input to.
*   var_size: the size of the variable to write to.
*/

void getInput(const char *msg, char *input_var, const int var_size)
{
    printf("%s", msg);
    fgets(input_var, var_size, stdin);

    /* Remove newline added by fgets */
    strtok(input_var, "\n");
}


/* BEGIN MAIN WRAPPER FUNCTIONS */

/*
*   Function: createFileMain
*   --------------------------
*   Wrapper for createFile().
*   Takes user input and creates a file given a name.
*
*   changelog_directory: the full path to the changelog directory.
*/

void createFileMain(const char *changelog_directory)
{
    char file_name[MAX_FILE_NAME_SIZE];
    int error;

    getInput("Enter the name of the file you want to create: ", file_name, sizeof(file_name));

    error = createFile(file_name);
    if (!error)
    {
        printf("Successully created file '%s'\n", file_name);
        addActionToChangelog(file_name, ACTION_CREATE_FILE, changelog_directory);
    }
}

/*
*   Function: displayFileMain
*   ---------------------------
*   Wrapper for openFile().
*   Takes user input and displays the contents of a given file.
*
*   changelog_directory: the full path to the changelog directory.
*/

void displayFileMain(const char *changelog_directory)
{
    char file_name[MAX_FILE_NAME_SIZE];
    int error;

    getInput("Enter the name of the file you want to see the contents of: ", file_name, sizeof(file_name));

    error = displayFile(file_name);

    if (error)
    {
        printf("\n[Error] Failed to display contents of '%s'. See above for more information.\n", file_name);
    }
    else
    {
        addActionToChangelog(file_name, ACTION_READ_FILE, changelog_directory);
    }
}

/*
*   Function: copyFileMain
*   ------------------------
*   Wrapper for copyFile().
*   Takes user input, copies the contents of one given file,
*   and writes it into a newly created file (given the name).
*
*   changelog_directory: the full path to the changelog directory.
*/

void copyFileMain(const char *changelog_directory)
{
    char source_file_name[MAX_FILE_NAME_SIZE];
    char new_file_name[MAX_FILE_NAME_SIZE];
    char *file_contents;
    int error;

    getInput("Enter the name of the file you want to copy: ", source_file_name, sizeof(source_file_name));
    getInput("Enter the name of your new file: ", new_file_name, sizeof(new_file_name));

    error = copyFile(source_file_name, new_file_name);
    if (!error)
    {
        printf("Successfully copied file '%s' to '%s'\n", source_file_name, new_file_name);
        addActionToChangelog(new_file_name, ACTION_CREATE_FILE, changelog_directory);
    }
}

/*
*   Function: deleteFileMain
*   --------------------------
*   Wrapper for deleteFile().
*   Takes user input and deletes the file with the given name.
*
*   changelog_directory: the full path to the changelog directory.
*/

void deleteFileMain(const char *changelog_directory)
{
    char file_name[MAX_FILE_NAME_SIZE];
    int error;

    getInput("Enter the name of the file you want to delete: ", file_name, sizeof(file_name));

    error = deleteFile(file_name);
    if (!error)
    {
        printf("Successfully deleted file '%s'\n", file_name);
        deleteFileFromChangelog(file_name, changelog_directory);
    }
}

/*
*   Function: appendLineMain
*   --------------------------
*   Wrapper for appendLineToFile().
*   Takes user input and appends the given content to the file with the given name.
*
*   changelog_directory: the full path to the changelog directory.
*/

void appendLineMain(const char *changelog_directory)
{
    char file_name[MAX_FILE_NAME_SIZE];
    char line_content[MAX_LINE_CONTENT_SIZE];
    int error;

    getInput("Enter the file you want to append content to: ", file_name, sizeof(file_name));
    getInput("Enter the content you want to append:\n", line_content, sizeof(line_content));

    error = appendLineToFile(file_name, line_content);
    if (!error)
    {
        printf("Sucessfully appended content to file '%s'\n", file_name);
        addActionToChangelog(file_name, ACTION_APPEND_LINE, changelog_directory);
    }
}

/*
*   Function: deleteLineMain
*   ------------------------
*   Wrapper for deleteLineFromFile().
*   Takes user input and deletes the specified line from the file.
*
*   changelog_directory: the full path to the changelog directory.
*/

void deleteLineMain(const char *changelog_directory)
{
    char file_name[MAX_FILE_NAME_SIZE];
    char line_number[DEFAULT_INPUT_BUFFER];
    int line_number_int;
    int error;

    getInput("Enter the file you want to delete a line from: ", file_name, sizeof(file_name));
    getInput("Enter the line number you want to delete: ", line_number, sizeof(line_number));

    /* Convert user input to an integer */
    line_number_int = atoi(line_number);

    error = deleteLineFromFile(file_name, line_number_int);
    if (!error)
    {
        printf("Successfully deleted line %d from '%s'\n", line_number_int, file_name);
        addActionToChangelog(file_name, ACTION_DELETE_LINE, changelog_directory);
    }
}

/*
*   Function: insertLineMain
*   ------------------------
*   Wrapper for insertLineInFile().
*   Takes user input and inserts content at the specified line number.
*
*   changelog_directory: the full path to the changelog directory.
*/

void insertLineMain(const char *changelog_directory)
{
    char file_name[MAX_FILE_NAME_SIZE];
    char line_number[DEFAULT_INPUT_BUFFER];
    char line_content[MAX_LINE_CONTENT_SIZE];
    int line_number_int;
    int error;

    getInput("Enter the file you want to insert a line into: ", file_name, sizeof(file_name));
    getInput("Enter the line number you want to insert content at: ", line_number, sizeof(line_number));
    getInput("Enter the content you want to insert: ", line_content, sizeof(line_content));

    /* Convert user input to an integer */
    line_number_int = atoi(line_number);

    error = insertLineInFile(file_name, line_content, line_number_int);
    if (!error)
    {
        printf("Successully inserted content at line %d in '%s'\n", line_number_int, file_name);
        addActionToChangelog(file_name, ACTION_INSERT_LINE, changelog_directory);
    }
}

/*
*   Function: showLineMain
*   ------------------------
*   Wrapper for showLineFromFile().
*   Takes user input and displays the contents of a file at a specified line number.
*
*   changelog_directory: the full path to the changelog directory.
*/

void showLineMain(const char *changelog_directory)
{
    char file_name[MAX_FILE_NAME_SIZE];
    char line_number[DEFAULT_INPUT_BUFFER];
    int line_number_int;
    int error;

    getInput("Enter the file you want to read a line from: ", file_name, sizeof(file_name));
    getInput("Ether the line number you want to read the contents at: ", line_number, sizeof(line_number));

    /* COnvert user input to an integer */
    line_number_int = atoi(line_number);

    error = showLineFromFile(file_name, line_number_int);
    if (!error)
    {
        addActionToChangelog(file_name, ACTION_READ_LINE, changelog_directory);
    }
}

/*
*   Function: getLinesMain
*   ------------------------
*   Wrapper for displayNumberOfLinesInFile().
*   Takes user input and counts the number of lines in a specified file.
*
*   changelog_directory: the full path to the changelog directory.
*/

void getLinesMain(const char *changelog_directory)
{
    char file_name[MAX_FILE_NAME_SIZE];
    int line_count;
    int error;

    getInput("Enter the file you want to count the number of lines from: ", file_name, sizeof(file_name));

    error = displayNumberOfLinesInFile(file_name);
    if (error)
    {
        printf("\n[Error] Failed to count lines in '%s'. See above for more information.\n", file_name);
    }
    else
    {
        addActionToChangelog(file_name, ACTION_READ_FILE, changelog_directory);
    }
}


/*
*   Function: getCurrentDirectoryMain
*   ------------------------
*   Displays all files in the current directory.
*
*   changelog_directory: the full path to the changelog directory.
*/

void getCurrentDirectoryMain(const char *changelog_directory)
{
    DIR *current_directory;
    struct dirent *directory_pointer;
    char current_file_name[MAX_FILE_NAME_SIZE];
    current_directory = opendir(".");
    if (current_directory)
    {
        printf("Files in current directory:\n");
        directory_pointer = readdir(current_directory);

        while (directory_pointer != NULL)
        {
            if (directory_pointer->d_name[0] != '.')
            {
                printf("%s\n", directory_pointer->d_name);
            }
            directory_pointer = readdir(current_directory);
        }

        closedir(current_directory);
    }
    else
    {
        fprintf(stderr, "\n[Error] Failed to open current directory: %s\n", strerror(errno));
    }
}

/*
*   Function: resetChangelogMain
*   -----------------------------
*   Wrapper for resetChangelog()
*   Takes user input and resets the changelog for a specified file
*
*   changelog_directory: the full path to the changelog directory.
*/

void resetChangelogMain(const char *changelog_directory)
{
    char file_name[MAX_FILE_NAME_SIZE];
    int error;

    getInput("Enter the file that you want to reset the changelog of: ", file_name, sizeof(file_name));

    error = resetChangelog(file_name, changelog_directory);
    if (!error)
    {
        printf("Successfully reset changelog for '%s'\n", file_name);
    }
}


/*
*   Function: showChangelogMain
*   ------------------------
*   Wrapper for showChangelog().
*   Shows the changelog for a specified file.
*
*   changelog_directory: the full path to the changelog directory.
*/

void showChangelogMain(const char *changelog_directory)
{
    char file_name[MAX_FILE_NAME_SIZE];
    int error;

    getInput("Enter the file you want to see the changelog of: ", file_name, sizeof(file_name));

    error = showChangelog(file_name, changelog_directory);

    if (error)
    {
        fprintf(stderr, "\n[Error] Failed to display changelog for file '%s': See above for more information.\n", file_name);
    }
}

/* END MAIN FUNCTIONS */

/*
*   Function: initaliseChangelog
*   ----------------------------
*   Checks if the changelog folder exists and creates it if it doesn't.
*   Will exit the program if there doesn't exist a readable changelog directory by the end.
*/

void initialiseChangelog()
{
    DIR *changelog = opendir(CHANGELOG_NAME);
    if (changelog)
    {
        /* Changelog exists, so clean up and move on */
        closedir(changelog);
    }
    else if (errno = ENOENT)
    {
        /* Changelog doesn't exist. Try to create it */
        printf("Creating directory '%s'...\n", CHANGELOG_NAME);
        if (mkdir(CHANGELOG_NAME, 0755))
        {
            perror("\n[Error]");
            fprintf(stderr, "\n[Error] Failed to create changelog directory '%s': See above for more information.\n", CHANGELOG_NAME);
            exit(1);
        }
        printf("Successfully created directory '%s'\n", CHANGELOG_NAME);
    }
    else
    {
        /* Failed to open changelog for reason other than ENOENT */
        perror("\n[Error]");
        fprintf(stderr, "\n[Error] Failed to open changelog directory '%s': See above for more information.\n", CHANGELOG_NAME);
        printf("Exiting program...");
        exit(1);
    }
}


/*
*   Function: showOptionsList
*   -------------------------
*   Outputs the list of options for the program
*/

void showOptionsList()
{
    printf("\n");
    printf("List of operations:\n");
    printf("0 - Show this message\n");
    printf("1 - Create a new file\n");
    printf("2 - Display the contents of a file\n");
    printf("3 - Copy a file\n");
    printf("4 - Delete a file\n");
    printf("5 - Append a line of content to a file\n");
    printf("6 - Delete a line of content at a certain line number\n");
    printf("7 - Insert a line of content at a certain line number\n");
    printf("8 - Display the contents of a file at a certain line number\n");
    printf("9 - Show the number of lines in a file\n");
    printf("10 - Get all files in the current directory\n");
    printf("11 - Reset the changelog for a file\n");
    printf("12 - Show the changelog for a file\n");
    printf("13 - Quit the program\n"); /* Spooky */
}


/* Main Function */
int main(int argc, char *argv[])
{

    initialiseChangelog();
    printf("\n");

    char operation[DEFAULT_INPUT_BUFFER];
    int operationInt;
    char term;

    /* Get and store current working directory */
    char cwd[MAX_FILE_PATH_SIZE];
    getcwd(cwd, sizeof(cwd));

    char changelog_directory[MAX_FILE_PATH_SIZE];
    sprintf(changelog_directory, "%s/%s", cwd, CHANGELOG_NAME);

    /* Array of pointers to our main functions */
    void (*functions[13])() = {
        showOptionsList,
        createFileMain,
        displayFileMain,
        copyFileMain,
        deleteFileMain,
        appendLineMain,
        deleteLineMain,
        insertLineMain,
        showLineMain,
        getLinesMain,
        getCurrentDirectoryMain,
        resetChangelogMain,
        showChangelogMain
    };

    printf("Welcome to the file manager!\n");
    printf("With this program, you can perform a variety of operations as shown below.\n");
    printf("All operations are only applicable on files in the current directory.\n");

    showOptionsList();
    printf("\n");

    while (1)
    {
        printf("Enter the operation you would like to perform (or '0' to display them again): ");

        fgets(operation, sizeof(operation), stdin);
        operationInt = atoi(operation);

        if (operationInt == 13)
        {
            printf("Quitting...\n");
            break;
        }
        else if (operationInt >= 0 && operationInt < 13)
        {
            /* Get the function pointer matching the index and call it */
            (*functions[operationInt])(changelog_directory);
            printf("\n");
        }
        else
        {
            printf("Invalid operation selected.\n");
            continue;
        }

    }

    return SUCCESS;

}