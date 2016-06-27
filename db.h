/********************************************************************
db.h - This file contains all the structures, defines, and function
	prototype for the db.exe program.
*********************************************************************/

#include <stdint.h>

#define MAX_IDENT_LEN   16
#define MAX_NUM_COL			16
#define MAX_TOK_LEN			32
#define MAX_STRING_LEN		255
#define KEYWORD_OFFSET	10
#define STRING_BREAK		" (),<>="
#define NUMBER_BREAK		" ),"
#define SELECT_STAR 9997
#define SELECT_COL_NAME 9998
#define SELECT_AGGR_FUNCT 9999
#define TIME_STAMP_LENGTH 14
#define LOG_RECORD_LENGTH 512
#define LOG_FILE_NAME "db.log"
#define TEMP_LOG_FILE "backuplog.log"

/* Column descriptor sturcture = 20+4+4+4+4 = 36 bytes */
typedef struct cd_entry_def {
	char		col_name[MAX_IDENT_LEN + 4];
	int			col_id;                   /* Start from 0 */
	int			col_type;
	int			col_len;
	int 		not_null;
} cd_entry;

/* Table packed descriptor sturcture = 4+20+4+4+4 = 36 bytes
   Minimum of 1 column in a table - therefore minimum size of
	 1 valid tpd_entry is 36+36 = 72 bytes. */
typedef struct tpd_entry_def {
	int				tpd_size;
	char			table_name[MAX_IDENT_LEN + 4];
	int				num_columns;
	int				cd_offset;
	int       tpd_flags;
} tpd_entry;

/* Table packed descriptor list = 4+4+4+36 = 48 bytes.  When no
   table is defined the tpd_list is 48 bytes.  When there is
	 at least 1 table, then the tpd_entry (36 bytes) will be
	 overlapped by the first valid tpd_entry. */
typedef struct tpd_list_def {
	int				list_size;
	int				num_tables;
	int				db_flags;
	tpd_entry	tpd_start;
}tpd_list;

/* This token_list definition is used for breaking the command
   string into separate tokens in function get_tokens().  For
	 each token, a new token_list will be allocated and linked
	 together. */
typedef struct t_list {
	char	tok_string[MAX_TOK_LEN];
	int		tok_class;
	int		tok_value;
	struct t_list *next;
} token_list;

/* Table file structure */
typedef struct table_file_header_def {
	int			file_size;			// 4 bytes
	int			record_size;			// 4 bytes
	int			num_records;			// 4 bytes
	int			record_offset;			// 4 bytes
	int			file_header_flag;		// 4 bytes
	tpd_entry		*tpd_ptr;			// 4 bytes
} table_file_header;					// minimum size = 24

/* A field for table insertion */
typedef struct field {
	char *value;
	int type;
	int tok_value;
	int length;
} field;

/* Column info used for printing */ 
typedef struct column_info_for_printing {
	char		*col_name;
	int			col_len;
	int			col_type;
	int         col_offset;
	struct column_info_for_printing *next;
} column_info;

/* Stores condition for a where clause */
typedef struct condition_for_where {
	char *col_name;
	int op; // relation operator >, <, =
	char *val;
	int and_or; // and = 0, or = 1
	struct condition_for_where *next_cond;
} condition;

/* This enum defines the different classes of tokens for
	 semantic processing. */
typedef enum t_class {
	keyword = 1,	// 1
	identifier,		// 2
	symbol, 			// 3
	type_name,		// 4
	constant,		  // 5
	function_name,// 6
	terminator,		// 7
	error			    // 8

} token_class;

/* This enum defines the different values associated with
   a single valid token.  Use for semantic processing. */
typedef enum t_value {
	T_INT = 10,		// 10 - new type should be added above this line
	T_CHAR,		    // 11 
	K_CREATE, 		// 12
	K_TABLE,			// 13
	K_NOT,				// 14
	K_NULL,				// 15
	K_DROP,				// 16
	K_LIST,				// 17
	K_SCHEMA,			// 18
	K_FOR,        // 19
	K_TO,				  // 20
	K_INSERT,     // 21
	K_INTO,       // 22
	K_VALUES,     // 23
	K_DELETE,     // 24
	K_FROM,       // 25
	K_WHERE,      // 26
	K_UPDATE,     // 27
	K_SET,        // 28
	K_SELECT,     // 29
	K_ORDER,      // 30
	K_BY,         // 31
	K_DESC,       // 32
	K_IS,         // 33
	K_AND,        // 34
	K_OR,         // 35 - new keyword should be added below this line
	K_BACKUP,     // 36  <= added
	K_RESTORE,    // 37  <= added
	K_WITHOUT,    // 38  <= added
	K_RF,         // 39  <= added
	K_ROLLFORWARD,// 40  <= added
	F_SUM,        // 41
	F_AVG,        // 42
	F_COUNT,      // 43 - new function name should be added below this line
	S_LEFT_PAREN = 70,  // 70
	S_RIGHT_PAREN,		  // 71
	S_COMMA,			      // 72
	S_STAR,             // 73
	S_EQUAL,            // 74
	S_LESS,             // 75
	S_GREATER,          // 76
	IDENT = 85,			    // 85
	INT_LITERAL = 90,	  // 90
	STRING_LITERAL,     // 91
	EOC = 95,			      // 95
	INVALID = 99		    // 99
} token_value;

/* This constants must be updated when add new keywords */
#define TOTAL_KEYWORDS_PLUS_TYPE_NAMES 34

/* New keyword must be added in the same position/order as the enum
   definition above, otherwise the lookup will be wrong */
char *keyword_table[] =
{
	"int", "char", "create", "table", "not", "null", "drop", "list", "schema",
	"for", "to", "insert", "into", "values", "delete", "from", "where",
	"update", "set", "select", "order", "by", "desc", "is", "and", "or",
	"backup", "restore", "without", "rf", "rollforward", "sum", "avg", "count"
};

/* This enum defines a set of possible statements */
typedef enum s_statement {
	INVALID_STATEMENT = -199,	// -199
	CREATE_TABLE = 100,				// 100
	DROP_TABLE,								// 101
	LIST_TABLE,								// 102
	LIST_SCHEMA,							// 103
	INSERT,                   // 104
	DELETE,                   // 105
	UPDATE,                   // 106
	SELECT,                    // 107
	BACKUP,
	RESTORE,
	ROLLFORWARD
} semantic_statement;

/* This enum has a list of all the errors that should be detected
   by the program.  Can append to this if necessary. */
typedef enum error_return_codes {
	INVALID_TABLE_NAME = -399,	// -399
	DUPLICATE_TABLE_NAME,				// -398
	TABLE_NOT_EXIST,						// -397
	INVALID_TABLE_DEFINITION,		// -396
	INVALID_COLUMN_NAME,				// -395
	DUPLICATE_COLUMN_NAME,			// -394
	COLUMN_NOT_EXIST,						// -393
	MAX_COLUMN_EXCEEDED,				// -392
	INVALID_TYPE_NAME,					// -391
	INVALID_COLUMN_DEFINITION,	// -390
	INVALID_COLUMN_LENGTH,			// -389
	INVALID_REPORT_FILE_NAME,		// -388
	/* Must add all the possible errors from I/U/D + SELECT here */
	FILE_OPEN_ERROR = -299,			// -299
	DBFILE_CORRUPTION,					// -298
	MEMORY_ERROR,						  // -297
	WRITE_TO_LOG_ERROR,
	INVALID_IMAGE_FILE_NAME,
	IMAGE_FILE_ALREADY_EXISTS,
	ROLLFORWARD_PENDING,
	COULD_NOT_RESTORE,
	INVALID_FIELD_VALUE,
	INVALID_INSERTION_SYNTAX,
	INVALID_DELETE_SYNTAX,
	INVALID_WHERE_SYNTAX,
	INVALID_UPDATE_SYNTAX,
	INVALID_ROLLFORWARD_SYNTAX,
	INVALID_TIMESTAMP,
	INVALID_DATA_VALUE,
	INVALID_TYPE_AGGREGATION,
	NOT_NULL_ENFORCED,
	TYPE_MISMATCH,
	STRING_TOO_LONG,
	DIFFERENT_NUM_COLUMNS,
	CORRUPTED_TABFILE,
	EXCEEDED_MAX_ROWS,
	INVALID_SELECT_SYNTAX,
	FILE_DELETE_ERROR, 
} return_codes;

/* Set of function prototypes */
int get_token(char *command, token_list **tok_list);
void add_to_list(token_list **tok_list, char *tmp, int t_class, int t_value);
int do_semantic(token_list *tok_list);
int sem_create_table(token_list *t_list);
int sem_drop_table(token_list *t_list);
int sem_list_tables();
int sem_list_schema(token_list *t_list);

/* Functions I added */
/* Check semantics */
int sem_where(token_list **t_list, tpd_entry *tpd_entry, condition **cond);
int sem_order_by(tpd_entry *tpd_entry, token_list **t_list, column_info **order_by_list, char **col_name, bool *desc);
int sem_insert_into(token_list *t_list);
int sem_delete_from(token_list *t_list);
int sem_update_table(token_list *t_list);
int sem_backup(token_list *t_list);
int sem_select_from(token_list *t_list);
int sem_backup(token_list *t_list);
int sem_restore(token_list *t_list);
int sem_rollforward(token_list *t_list);

/* Process create, insert, delete, update */
int create_table_file(char *table_name, int record_size);
int add_row_to_table(field *field_list, char *table_name, int num_cols);
void select(char *table, column_info *col_info_list, int num_records, int record_size);
void select_aggr_funct(char *table, column_info *col_info_list, int aggr_funct, char *col_name, int num_records, int record_size);
int delete_from_table(tpd_entry *entry, condition *cond);
int update_table(tpd_entry *tpd_ptr, char* col_name, char *set_value, condition *cond);
int backup_image(char *image_name);
int restore_image(char *image_name);
int rollforward(char *time_stamp);
/* Helper functions */
int toggle_dbflag();
int restore_tab_file(table_file_header *tfh_entry, char *table_name);
int redo_logged(char *query);
char* get_backup_log_name();
char *make_time_stamp();
int append_to_log(char *entry, char *image_name);
char *get_records_where(char *table, cd_entry *col_entry, condition *cond, int num_cols, int *num_records, int *record_size);
int compare_desc(const void* rec1, const void* rec2);
int compare_asc(const void* rec1, const void* rec2);
char *get_records_order(char *table, column_info *order_by_list, int *num_records, int *record_size, bool desc);
void sort_table(char *table, int num_records, int record_size, bool desc);
void add_to_col_info_list(column_info **clist, char *col_name, int type, int length, int offset);
int update_helper(char *first_record, char *last_record, table_file_header *tfh_entry,
	tpd_entry *tpd_ptr, char *col_name, char *set_value, condition *cond, int *count);
int set_column(char **record, cd_entry *col_entry, char *set_value);
int check_col_name(tpd_entry *tpd_entry, char *name, int *type, int *length, int *offset);
int check_type_limits(token_list *cur, int col_type);
int check_value_against_cd_def(tpd_entry *tpd_entry, char *col_name_from_where, token_list *cur);
bool check_condition(cd_entry *col_entry, int num_columns, char *record, condition *cond);
bool evaluate_condition(char *record, int type, uint8_t col_length, condition *cond);
void free_conditions(condition *cond);
void add_tab_extension(char *tf_name, char *table_name);
table_file_header* get_records_from_tab_file(tpd_entry *tpd_entry);
/* Variables used for comparator */
int type_for_qsort;
int offset_for_qsort;
/* Variables for log */
char time_buffer[TIME_STAMP_LENGTH + 1];
char log_buffer[LOG_RECORD_LENGTH + 1];
bool without_rf;
char *query;

/*
	Keep a global list of tpd - in real life, this will be stored
	in shared memory.  Build a set of functions/methods around this.
*/
tpd_list	*g_tpd_list;
int initialize_tpd_list();
int add_tpd_to_list(tpd_entry *tpd);
int drop_tpd_from_list(char *tabname);
tpd_entry* get_tpd_from_list(char *tabname);

