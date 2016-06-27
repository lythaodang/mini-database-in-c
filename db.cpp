/************************************************************
	Project#1:	CLP & DDL
 ************************************************************/

#include "db.h"
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <ctype.h>
#include <limits.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <time.h>

#define max(a, b)  (((a) > (b)) ? (a) : (b))

int main(int argc, char** argv) {
	int rc = 0;
	token_list *tok_list = NULL, *tok_ptr = NULL, *tmp_tok_ptr = NULL;

	if ((argc != 2) || (strlen(argv[1]) == 0)) {
		printf("Usage: db \"command statement\"");
		return 1;
	}

	rc = initialize_tpd_list();

	if (rc) {
		printf("\nError in initialize_tpd_list().\nrc = %d\n", rc);
	} else {
		rc = get_token(argv[1], &tok_list);
		query = argv[1];

		/* Test code */
		/*
		tok_ptr = tok_list;
		while (tok_ptr != NULL) {
			printf("%16s \t%d \t %d\n", tok_ptr->tok_string, tok_ptr->tok_class,
				tok_ptr->tok_value);
			tok_ptr = tok_ptr->next;
		}
		*/

		if (!rc) {
			rc = do_semantic(tok_list);
		}

		if (rc) {
			tok_ptr = tok_list;
			while (tok_ptr != NULL) {
				if ((tok_ptr->tok_class == error) ||
					(tok_ptr->tok_value == INVALID)) {
					printf("\nError in the string: %s\n", tok_ptr->tok_string);
					printf("rc=%d\n", rc);
					break;
				}
				tok_ptr = tok_ptr->next;
			}
		}

		/* Whether the token list is valid or not, we need to free the memory */
		tok_ptr = tok_list;
		while (tok_ptr != NULL) {
			tmp_tok_ptr = tok_ptr->next;
			free(tok_ptr);
			tok_ptr = tmp_tok_ptr;
		}
	}

	return rc;
}

/*************************************************************
	This is a lexical analyzer for simple SQL statements
 *************************************************************/
int get_token(char* command, token_list** tok_list) {
	int rc = 0, i, j;
	char *start, *cur, temp_string[MAX_TOK_LEN];
	bool done = false;

	start = cur = command;
	while (!done) {
		bool found_keyword = false;

		/* This is the TOP Level for each token */
		memset((void*) temp_string, '\0', MAX_TOK_LEN);
		i = 0;

		/* Get rid of all the leading blanks */
		while (*cur == ' ')
			cur++;

		if (cur && isalpha(*cur)) {
			// find valid identifier
			int t_class;
			do {
				temp_string[i++] = *cur++;
			} while ((isalnum(*cur)) || (*cur == '_'));

			if (!(strchr(STRING_BREAK, *cur))) {
				/* If the next char following the keyword or identifier
					 is not a blank, (, ), or a comma, then append this
					 character to temp_string, and flag this as an error */
				temp_string[i++] = *cur++;
				add_to_list(tok_list, temp_string, error, INVALID);
				rc = INVALID;
				done = true;
			} else {

				// We have an identifier with at least 1 character
				// Now check if this ident is a keyword
				for (j = 0, found_keyword = false; j < TOTAL_KEYWORDS_PLUS_TYPE_NAMES; j++) {
					if ((stricmp(keyword_table[j], temp_string) == 0)) {
						found_keyword = true;
						break;
					}
				}

				if (found_keyword) {
					if (KEYWORD_OFFSET + j < K_CREATE)
						t_class = type_name;
					else if (KEYWORD_OFFSET + j >= F_SUM)
						t_class = function_name;
					else
						t_class = keyword;

					add_to_list(tok_list, temp_string, t_class, KEYWORD_OFFSET + j);
				} else {
					if (strlen(temp_string) <= MAX_IDENT_LEN)
						add_to_list(tok_list, temp_string, identifier, IDENT);
					else {
						add_to_list(tok_list, temp_string, error, INVALID);
						rc = INVALID;
						done = true;
					}
				}

				if (!*cur) {
					add_to_list(tok_list, "", terminator, EOC);
					done = true;
				}
			}
		} else if (isdigit(*cur)) {
			// find valid number
			do {
				temp_string[i++] = *cur++;
			} while (isdigit(*cur));

			if (!(strchr(NUMBER_BREAK, *cur))) {
				/* If the next char following the keyword or identifier
					 is not a blank or a ), then append this
					 character to temp_string, and flag this as an error */
				temp_string[i++] = *cur++;
				add_to_list(tok_list, temp_string, error, INVALID);
				rc = INVALID;
				done = true;
			} else {
				add_to_list(tok_list, temp_string, constant, INT_LITERAL);

				if (!*cur) {
					add_to_list(tok_list, "", terminator, EOC);
					done = true;
				}
			}
		} else if ((*cur == '(') || (*cur == ')') || (*cur == ',') || (*cur == '*')
			|| (*cur == '=') || (*cur == '<') || (*cur == '>')) {
			/* Catch all the symbols here. Note: no look ahead here. */
			int t_value;
			switch (*cur) {
			case '(':
				t_value = S_LEFT_PAREN;
				break;
			case ')':
				t_value = S_RIGHT_PAREN;
				break;
			case ',':
				t_value = S_COMMA;
				break;
			case '*':
				t_value = S_STAR;
				break;
			case '=':
				t_value = S_EQUAL;
				break;
			case '<':
				t_value = S_LESS;
				break;
			case '>':
				t_value = S_GREATER;
				break;
			}

			temp_string[i++] = *cur++;

			add_to_list(tok_list, temp_string, symbol, t_value);

			if (!*cur) {
				add_to_list(tok_list, "", terminator, EOC);
				done = true;
			}
		} else if (*cur == '\'') {
			/* Find STRING_LITERRAL */
			cur++;
			do {
				temp_string[i++] = *cur++;
			} while ((*cur) && (*cur != '\''));

			temp_string[i] = '\0';

			if (!*cur) {
				/* If we reach the end of line */
				add_to_list(tok_list, temp_string, error, INVALID);
				rc = INVALID;
				done = true;
			} else /* must be a ' */
			{
				add_to_list(tok_list, temp_string, constant, STRING_LITERAL);
				cur++;
				if (!*cur) {
					add_to_list(tok_list, "", terminator, EOC);
					done = true;
				}
			}
		} else {
			if (!*cur) {
				add_to_list(tok_list, "", terminator, EOC);
				done = true;
			} else {
				/* not a ident, number, or valid symbol */
				temp_string[i++] = *cur++;
				add_to_list(tok_list, temp_string, error, INVALID);
				rc = INVALID;
				done = true;
			}
		}
	}

	return rc;
}

void add_to_list(token_list **tok_list, char *tmp, int t_class, int t_value) {
	token_list *cur = *tok_list;
	token_list *ptr = NULL;

	// printf("%16s \t%d \t %d\n",tmp, t_class, t_value);

	ptr = (token_list*) calloc(1, sizeof(token_list));
	strcpy(ptr->tok_string, tmp);
	ptr->tok_class = t_class;
	ptr->tok_value = t_value;
	ptr->next = NULL;

	if (cur == NULL)
		*tok_list = ptr;
	else {
		while (cur->next != NULL)
			cur = cur->next;

		cur->next = ptr;
	}
	return;
}

int do_semantic(token_list *tok_list) {
	int rc = 0, cur_cmd = INVALID_STATEMENT;
	bool unique = false;
	token_list *cur = tok_list;

	if (cur->tok_value == K_SELECT) {
		printf("SELECT statement\n");
		cur_cmd = SELECT;
		cur = cur->next;
	} else if (g_tpd_list->db_flags == ROLLFORWARD_PENDING) {
		if (cur->tok_value == K_ROLLFORWARD) {
			printf("ROLLFORWARD statement\n");
			cur_cmd = ROLLFORWARD;
			cur = cur->next;
		} else {
			printf("ERROR: RF pending\n");
			rc = ROLLFORWARD_PENDING;
			cur = cur->next;
		}
	} else if ((cur->tok_value == K_CREATE) &&
		((cur->next != NULL) && (cur->next->tok_value == K_TABLE))) {
		printf("CREATE TABLE statement\n");
		cur_cmd = CREATE_TABLE;
		cur = cur->next->next;
	} else if ((cur->tok_value == K_DROP) &&
		((cur->next != NULL) && (cur->next->tok_value == K_TABLE))) {
		printf("DROP TABLE statement\n");
		cur_cmd = DROP_TABLE;
		cur = cur->next->next;
	} else if ((cur->tok_value == K_LIST) &&
		((cur->next != NULL) && (cur->next->tok_value == K_TABLE))) {
		printf("LIST TABLE statement\n");
		cur_cmd = LIST_TABLE;
		cur = cur->next->next;
	} else if ((cur->tok_value == K_LIST) &&
		((cur->next != NULL) && (cur->next->tok_value == K_SCHEMA))) {
		printf("LIST SCHEMA statement\n");
		cur_cmd = LIST_SCHEMA;
		cur = cur->next->next;
	} else if ((cur->tok_value == K_INSERT) &&
		((cur->next != NULL) && (cur->next->tok_value == K_INTO))) {
		printf("INSERT statement\n");
		cur_cmd = INSERT;
		cur = cur->next->next; 
	} else if ((cur->tok_value == K_DELETE) &&
		((cur->next != NULL) && (cur->next->tok_value == K_FROM))) {
		printf("DELETE FROM statement\n");
		cur_cmd = DELETE;
		cur = cur->next->next;
	} else if (cur->tok_value == K_UPDATE) {
		printf("UPDATE TABLE statement\n");
		cur_cmd = UPDATE;
		cur = cur->next;
	} else if ((cur->tok_value == K_BACKUP) &&
		((cur->next != NULL) && (cur->next->tok_value == K_TO))) {
		printf("BACKUP TO statement\n");
		cur_cmd = BACKUP;
		cur = cur->next->next;
	} else if ((cur->tok_value == K_RESTORE) &&
		((cur->next != NULL) && (cur->next->tok_value == K_FROM))) {
		printf("RESTORE FROM statement\n");
		cur_cmd = RESTORE;
		cur = cur->next->next;
	} else {
		if (cur->tok_value == K_ROLLFORWARD) {
			printf("ERROR: RF_START missing cannot rollforward.\n");
		} else {
			printf("Invalid statement\n");
		}
		rc = cur_cmd;
	}

	if (cur_cmd != INVALID_STATEMENT) {
		switch (cur_cmd) {
		case CREATE_TABLE:
			rc = sem_create_table(cur);
			break;
		case DROP_TABLE:
			rc = sem_drop_table(cur);
			break;
		case LIST_TABLE:
			rc = sem_list_tables();
			break;
		case LIST_SCHEMA:
			rc = sem_list_schema(cur);
			break;
		case INSERT:
			rc = sem_insert_into(cur);
			break;
		case SELECT:
			rc = sem_select_from(cur);
			break;
		case DELETE:
			rc = sem_delete_from(cur);
			break;
		case UPDATE:
			rc = sem_update_table(cur);
			break;
		case BACKUP:
			sem_backup(cur);
			break;
		case RESTORE:
			sem_restore(cur);
			break;
		case ROLLFORWARD:
			sem_rollforward(cur);
		default:
			; /* no action */
		}
	}

	if (!rc && cur_cmd != BACKUP && cur_cmd != RESTORE && cur_cmd != ROLLFORWARD && cur_cmd != SELECT) {
		if (strnicmp(query, "rollforward", 11) != 0) {
			append_to_log(query, NULL);
		}
	}

	return rc;
}

/*************************************************************
Run semantics validation for where clause
*************************************************************/
int sem_where(token_list **t_list, tpd_entry *tpd_entry, condition **cond) {
	int rc = 0;
	token_list *cur = *t_list;

	cur = cur->next;
	if ((cur != NULL) && 
		(cur->tok_class != keyword) &&
		(cur->tok_class != identifier) &&
		(cur->tok_class != type_name)) {
		rc = INVALID_COLUMN_NAME;
		cur->tok_class = INVALID;
		printf("Invalid column name.\n");
	} else {
		int col_type = 0;
		rc = check_col_name(tpd_entry, cur->tok_string, &col_type, NULL, NULL);
		if (rc) {
			rc = INVALID_COLUMN_NAME;
			cur->tok_value = INVALID;
			printf("Column name not found.\n");
		} else {
			*cond = (condition*) calloc(1, sizeof(condition));
			(*cond)->col_name = cur->tok_string;

			cur = cur->next;
			// expect operator
			if (cur != NULL &&
				cur->tok_value != S_EQUAL &&
				cur->tok_value != S_LESS &&
				cur->tok_value != S_GREATER &&
				cur->tok_value != K_IS) {
				rc = INVALID_WHERE_SYNTAX;
				cur->tok_value = INVALID;
				printf("Invalid syntax. Relational operator expected.\n");
			} else {
				if (cur->tok_value == K_IS) {
					if (cur->next != NULL &&
						cur->next->tok_value != K_NOT &&
						cur->next->tok_value != K_NULL) {
						rc = INVALID_WHERE_SYNTAX;
						cur->next->tok_value = INVALID;
						printf("Invalid syntax. Expected IS NULL or IS NOT\n");
						return rc;
					} else if (cur->next != NULL &&
						cur->next->tok_value == K_NOT) {
						(*cond)->op = cur->tok_value + cur->next->tok_value;
						cur = cur->next;
					} else {
						(*cond)->op = cur->tok_value;
					}

					cur = cur->next;
					if (cur->tok_value != K_NULL) {
						rc = INVALID_WHERE_SYNTAX;
						cur->tok_value = INVALID;
						printf("Invalid syntax. Null expected.\n");
					} else {
						(*cond)->val = cur->tok_string;
					}
				} else {
					(*cond)->op = cur->tok_value;
					cur = cur->next;
					if ((cur != NULL) &&
						(cur->tok_value != INT_LITERAL) &&
						(cur->tok_value != STRING_LITERAL)) {
						rc = INVALID_WHERE_SYNTAX;
						cur->tok_value = INVALID;
						printf("Invalid syntax. Expecting an int or a string.\n");
					} else if ((cur != NULL) &&
						((col_type == T_INT && cur->tok_value != INT_LITERAL) ||
						(col_type == T_CHAR && cur->tok_value != STRING_LITERAL))) {
						rc = TYPE_MISMATCH;
						cur->tok_value = INVALID;
						printf("Type mismatch.\n");
					} else {
						rc = check_type_limits(cur, col_type);
						if (rc) {
							cur->tok_value = INVALID;
							printf("Data exceeds allowed limits.\n");
							return rc;
						}
						(*cond)->val = cur->tok_string;
					}
				}

				rc = check_value_against_cd_def(tpd_entry, (*cond)->col_name, cur);

				if (rc) {
					printf("Type mismatch or not null enforced.\n");
					return rc;
				}

				cur = cur->next;

				if (cur->tok_value != EOC &&
					cur->tok_value != K_AND &&
					cur->tok_value != K_OR &&
					cur->tok_value != K_ORDER) {
					rc = INVALID_WHERE_SYNTAX;
					cur->tok_value = INVALID;
					printf("Invalid syntax. AND or OR or ORDER expected.\n");
				} else if (cur->tok_value == EOC || cur->tok_value == K_ORDER) {
					rc = 0;
					*t_list = cur;
				} else {
					(*cond)->and_or = (cur->tok_value == K_AND) ? 0 : 1;
					rc = sem_where(&cur, tpd_entry, &((*cond)->next_cond));
					*t_list = cur;
				}
			}
		} 
	}
	return rc;
}

/*************************************************************
Run semantics validation for order by clause
*************************************************************/
int sem_order_by(tpd_entry *tpd_entry, token_list **t_list, column_info **order_by_list, char **col_name, bool *desc) {
	int rc = 0;
	token_list *cur = *t_list;
	char *temp_name = *col_name;
	cur = cur->next->next;
	
	// get column name
	if ((cur->tok_class != keyword) &&
		(cur->tok_class != identifier) &&
		(cur->tok_class != type_name)) {
		rc = INVALID_SELECT_SYNTAX;
		cur->tok_value = INVALID;
		printf("Invalid select syntax.\n");
	} else {
		temp_name = cur->tok_string;
		add_to_col_info_list(order_by_list, temp_name, 0, 0, 0);
		rc = check_col_name(tpd_entry, temp_name, &((*order_by_list)->col_type), &((*order_by_list)->col_len), &((*order_by_list)->col_offset));
		if (rc) {
			rc = INVALID_COLUMN_NAME;
			cur->tok_value = INVALID;
			printf("Invalid column name.\n");
		} else {
			cur = cur->next;
			if (cur->tok_value == EOC) { // without desc
				*desc = false;
			} else if (cur->tok_value == K_DESC) { // with desc
				*desc = true;
				cur = cur->next;
				if (cur->tok_value != EOC) {
					rc = INVALID_SELECT_SYNTAX;
					cur->tok_value = INVALID;
					printf("Invalid select syntax.\n");
				}
			} else {
				rc = INVALID_SELECT_SYNTAX;
				cur->tok_value = INVALID;
				printf("Invalid select syntax.\n");
			}
		}
	}

	*t_list = cur;
	*col_name = temp_name;

	return rc;
}

/*************************************************************
(1) Create table
*************************************************************/
int sem_create_table(token_list *t_list) {
	int rc = 0;
	token_list *cur;
	tpd_entry tab_entry;
	tpd_entry *new_entry = NULL;
	bool column_done = false;
	int cur_id = 0;
	cd_entry	col_entry[MAX_NUM_COL];


	memset(&tab_entry, '\0', sizeof(tpd_entry));
	cur = t_list;
	if ((cur->tok_class != keyword) &&
		(cur->tok_class != identifier) &&
		(cur->tok_class != type_name)) {
		// Error
		rc = INVALID_TABLE_NAME;
		cur->tok_value = INVALID;
	} else {
		if ((new_entry = get_tpd_from_list(cur->tok_string)) != NULL) {
			rc = DUPLICATE_TABLE_NAME;
			cur->tok_value = INVALID;
		} else {
			strcpy(tab_entry.table_name, cur->tok_string);
			cur = cur->next;
			if (cur->tok_value != S_LEFT_PAREN) {
				//Error
				rc = INVALID_TABLE_DEFINITION;
				cur->tok_value = INVALID;
			} else {
				memset(&col_entry, '\0', (MAX_NUM_COL * sizeof(cd_entry)));

				/* Now build a set of column entries */
				cur = cur->next;
				do {
					if ((cur->tok_class != keyword) &&
						(cur->tok_class != identifier) &&
						(cur->tok_class != type_name)) {
						// Error
						rc = INVALID_COLUMN_NAME;
						cur->tok_value = INVALID;
					} else {
						int i;
						for (i = 0; i < cur_id; i++) {
							/* make column name case sensitive */
							if (strcmp(col_entry[i].col_name, cur->tok_string) == 0) {
								rc = DUPLICATE_COLUMN_NAME;
								cur->tok_value = INVALID;
								break;
							}
						}

						if (!rc) {
							strcpy(col_entry[cur_id].col_name, cur->tok_string);
							col_entry[cur_id].col_id = cur_id;
							col_entry[cur_id].not_null = false;    /* set default */

							cur = cur->next;
							if (cur->tok_class != type_name) {
								// Error
								rc = INVALID_TYPE_NAME;
								cur->tok_value = INVALID;
							} else {
								/* Set the column type here, int or char */
								col_entry[cur_id].col_type = cur->tok_value;
								cur = cur->next;

								if (col_entry[cur_id].col_type == T_INT) {
									if ((cur->tok_value != S_COMMA) &&
										(cur->tok_value != K_NOT) &&
										(cur->tok_value != S_RIGHT_PAREN)) {
										rc = INVALID_COLUMN_DEFINITION;
										cur->tok_value = INVALID;
									} else {
										col_entry[cur_id].col_len = sizeof(int);

										if ((cur->tok_value == K_NOT) &&
											(cur->next->tok_value != K_NULL)) {
											rc = INVALID_COLUMN_DEFINITION;
											cur->tok_value = INVALID;
										} else if ((cur->tok_value == K_NOT) &&
											(cur->next->tok_value == K_NULL)) {
											col_entry[cur_id].not_null = true;
											cur = cur->next->next;
										}

										if (!rc) {
											/* I must have either a comma or right paren */
											if ((cur->tok_value != S_RIGHT_PAREN) &&
												(cur->tok_value != S_COMMA)) {
												rc = INVALID_COLUMN_DEFINITION;
												cur->tok_value = INVALID;
											} else {
												if (cur->tok_value == S_RIGHT_PAREN) {
													column_done = true;
												}
												cur = cur->next;
											}
										}
									}
								}   // end of S_INT processing
								else {
									// It must be char()
									if (cur->tok_value != S_LEFT_PAREN) {
										rc = INVALID_COLUMN_DEFINITION;
										cur->tok_value = INVALID;
									} else {
										/* Enter char(n) processing */
										cur = cur->next;

										if (cur->tok_value != INT_LITERAL) {
											rc = INVALID_COLUMN_LENGTH;
											cur->tok_value = INVALID;
										} else {
											/* Got a valid integer - convert */
											col_entry[cur_id].col_len = atoi(cur->tok_string);
											cur = cur->next;

											if (cur->tok_value != S_RIGHT_PAREN) {
												rc = INVALID_COLUMN_DEFINITION;
												cur->tok_value = INVALID;
											} else {
												cur = cur->next;

												if ((cur->tok_value != S_COMMA) &&
													(cur->tok_value != K_NOT) &&
													(cur->tok_value != S_RIGHT_PAREN)) {
													rc = INVALID_COLUMN_DEFINITION;
													cur->tok_value = INVALID;
												} else {
													if ((cur->tok_value == K_NOT) &&
														(cur->next->tok_value != K_NULL)) {
														rc = INVALID_COLUMN_DEFINITION;
														cur->tok_value = INVALID;
													} else if ((cur->tok_value == K_NOT) &&
														(cur->next->tok_value == K_NULL)) {
														col_entry[cur_id].not_null = true;
														cur = cur->next->next;
													}

													if (!rc) {
														/* I must have either a comma or right paren */
														if ((cur->tok_value != S_RIGHT_PAREN) && (cur->tok_value != S_COMMA)) {
															rc = INVALID_COLUMN_DEFINITION;
															cur->tok_value = INVALID;
														} else {
															if (cur->tok_value == S_RIGHT_PAREN) {
																column_done = true;
															}
															cur = cur->next;
														}
													}
												}
											}
										}	/* end char(n) processing */
									}
								} /* end char processing */
							}
						}  // duplicate column name
					} // invalid column name

					/* If rc=0, then get ready for the next column */
					if (!rc) {
						cur_id++;
					}

				} while ((rc == 0) && (!column_done));

				if ((column_done) && (cur->tok_value != EOC)) {
					rc = INVALID_TABLE_DEFINITION;
					cur->tok_value = INVALID;
				}

				if (!rc) {
					/* Now finished building tpd and add it to the tpd list */
					tab_entry.num_columns = cur_id;
					tab_entry.tpd_size = sizeof(tpd_entry) +
						sizeof(cd_entry) *	tab_entry.num_columns;
					tab_entry.cd_offset = sizeof(tpd_entry);
					new_entry = (tpd_entry*) calloc(1, tab_entry.tpd_size);

					if (new_entry == NULL) {
						rc = MEMORY_ERROR;
					} else {
						memcpy((void*) new_entry,
							(void*) &tab_entry,
							sizeof(tpd_entry));

						memcpy((void*) ((char*) new_entry + sizeof(tpd_entry)),
							(void*) col_entry,
							sizeof(cd_entry) * tab_entry.num_columns);

						rc = add_tpd_to_list(new_entry);

						free(new_entry);
					}

					/* create a <tablename>.tab file if there has not been an error */
					if (!rc) {
						int record_size = 0;
						int i = 0;

						/* get the record size based on num of cols */
						for (i = 0; i < tab_entry.num_columns; i++) {
							record_size += 1 + col_entry[i].col_len;
						}

						rc = create_table_file(tab_entry.table_name, record_size);
						if (!rc) {
							printf("%s.tab successfully created.\n", tab_entry.table_name);
						}
					}
				}
			}
		}
	}
	return rc;
}

/*************************************************************
(2) Drop table
*************************************************************/
int sem_drop_table(token_list *t_list) {
	int rc = 0;
	token_list *cur;
	tpd_entry *tab_entry = NULL;

	cur = t_list;
	if ((cur->tok_class != keyword) &&
		(cur->tok_class != identifier) &&
		(cur->tok_class != type_name)) {
		// Error
		rc = INVALID_TABLE_NAME;
		cur->tok_value = INVALID;
	} else {
		if (cur->next->tok_value != EOC) {
			rc = INVALID_STATEMENT;
			cur->next->tok_value = INVALID;
		} else {
			if ((tab_entry = get_tpd_from_list(cur->tok_string)) == NULL) {
				rc = TABLE_NOT_EXIST;
				cur->tok_value = INVALID;
			} else {
				char tf_name[MAX_IDENT_LEN + 5]; // MAX_IDENT_LEN + ".tab" extension
				add_tab_extension(tf_name, cur->tok_string);
				rc = remove(tf_name);

				if (!rc) {
					/* Found a valid tpd, drop it from tpd list */
					rc = drop_tpd_from_list(cur->tok_string);
					printf("DROP TABLE %s successful\n", cur->tok_string);
				} else {
					rc = FILE_DELETE_ERROR;
				}
			}
		}
	}

	return rc;
}

/*************************************************************
(3) List tables
*************************************************************/
int sem_list_tables() {
	int rc = 0;
	int num_tables = g_tpd_list->num_tables;
	tpd_entry *cur = &(g_tpd_list->tpd_start);

	if (num_tables == 0) {
		printf("\nThere are currently no tables defined\n");
	} else {
		printf("\nTable List\n");
		printf("*****************\n");
		while (num_tables-- > 0) {
			printf("%s\n", cur->table_name);
			if (num_tables > 0) {
				cur = (tpd_entry*) ((char*) cur + cur->tpd_size);
			}
		}
		printf("****** End ******\n");
	}

	return rc;
}

/*************************************************************
(4) List schema for table
*************************************************************/
int sem_list_schema(token_list *t_list) {
	int rc = 0;
	token_list *cur;
	tpd_entry *tab_entry = NULL;
	cd_entry  *col_entry = NULL;
	char tab_name[MAX_IDENT_LEN + 1];
	char filename[MAX_IDENT_LEN + 1];
	bool report = false;
	FILE *fhandle = NULL;
	int i = 0;

	cur = t_list;

	if (cur->tok_value != K_FOR) {
		rc = INVALID_STATEMENT;
		cur->tok_value = INVALID;
	} else {
		cur = cur->next;

		if ((cur->tok_class != keyword) &&
			(cur->tok_class != identifier) &&
			(cur->tok_class != type_name)) {
			// Error
			rc = INVALID_TABLE_NAME;
			cur->tok_value = INVALID;
		} else {
			memset(filename, '\0', MAX_IDENT_LEN + 1);
			strcpy(tab_name, cur->tok_string);
			cur = cur->next;

			if (cur->tok_value != EOC) {
				if (cur->tok_value == K_TO) {
					cur = cur->next;

					if ((cur->tok_class != keyword) &&
						(cur->tok_class != identifier) &&
						(cur->tok_class != type_name)) {
						// Error
						rc = INVALID_REPORT_FILE_NAME;
						cur->tok_value = INVALID;
					} else {
						if (cur->next->tok_value != EOC) {
							rc = INVALID_STATEMENT;
							cur->next->tok_value = INVALID;
						} else {
							/* We have a valid file name */
							strcpy(filename, cur->tok_string);
							report = true;
						}
					}
				} else {
					/* Missing the TO keyword */
					rc = INVALID_STATEMENT;
					cur->tok_value = INVALID;
				}
			}

			if (!rc) {
				if ((tab_entry = get_tpd_from_list(tab_name)) == NULL) {
					rc = TABLE_NOT_EXIST;
					cur->tok_value = INVALID;
				} else {
					if (report) {
						if ((fhandle = fopen(filename, "a+tc")) == NULL) {
							rc = FILE_OPEN_ERROR;
						}
					}

					if (!rc) {
						/* Find correct tpd, need to parse column and index information */

						/* First, write the tpd_entry information */
						printf("Table PD size            (tpd_size)    = %d\n", tab_entry->tpd_size);
						printf("Table Name               (table_name)  = %s\n", tab_entry->table_name);
						printf("Number of Columns        (num_columns) = %d\n", tab_entry->num_columns);
						printf("Column Descriptor Offset (cd_offset)   = %d\n", tab_entry->cd_offset);
						printf("Table PD Flags           (tpd_flags)   = %d\n\n", tab_entry->tpd_flags);

						if (report) {
							fprintf(fhandle, "Table PD size            (tpd_size)    = %d\n", tab_entry->tpd_size);
							fprintf(fhandle, "Table Name               (table_name)  = %s\n", tab_entry->table_name);
							fprintf(fhandle, "Number of Columns        (num_columns) = %d\n", tab_entry->num_columns);
							fprintf(fhandle, "Column Descriptor Offset (cd_offset)   = %d\n", tab_entry->cd_offset);
							fprintf(fhandle, "Table PD Flags           (tpd_flags)   = %d\n\n", tab_entry->tpd_flags);
						}

						/* Next, write the cd_entry information */
						for (i = 0, col_entry = (cd_entry*) ((char*) tab_entry + tab_entry->cd_offset);
							i < tab_entry->num_columns; i++, col_entry++) {
							printf("Column Name   (col_name) = %s\n", col_entry->col_name);
							printf("Column Id     (col_id)   = %d\n", col_entry->col_id);
							printf("Column Type   (col_type) = %d\n", col_entry->col_type);
							printf("Column Length (col_len)  = %d\n", col_entry->col_len);
							printf("Not Null flag (not_null) = %d\n\n", col_entry->not_null);

							if (report) {
								fprintf(fhandle, "Column Name   (col_name) = %s\n", col_entry->col_name);
								fprintf(fhandle, "Column Id     (col_id)   = %d\n", col_entry->col_id);
								fprintf(fhandle, "Column Type   (col_type) = %d\n", col_entry->col_type);
								fprintf(fhandle, "Column Length (col_len)  = %d\n", col_entry->col_len);
								fprintf(fhandle, "Not Null Flag (not_null) = %d\n\n", col_entry->not_null);
							}
						}

						if (report) {
							fflush(fhandle);
							fclose(fhandle);
						}
					} // File open error							
				} // Table not exist
			} // no semantic errors
		} // Invalid table name
	} // Invalid statement

	return rc;
}

/*************************************************************
(5) Insert into table
*************************************************************/
int sem_insert_into(token_list *t_list) {
	int rc = 0; // return code (0 = no error)
	token_list *cur = t_list;
	tpd_entry *tpd_entry = NULL;
	cd_entry  *col_entry = NULL;
	bool row_done = false;
	field field_list[MAX_NUM_COL];
	int field_count = 0;

	// tok_class must be a keyword, identifier, or type_name
	if ((cur->tok_class != keyword) && 
		(cur->tok_class != identifier) && 
		(cur->tok_class != type_name)) { 
		// Error - invalid table name
		rc = INVALID_TABLE_NAME;
		cur->tok_value = INVALID;
		printf("Invalid table name.\n");
	} else { 
		// check if table exists
		if ((tpd_entry = get_tpd_from_list(cur->tok_string)) == NULL) {
			// Error - cannot insert into a nonexistent table
			rc = TABLE_NOT_EXIST;
			cur->tok_value = INVALID;
			printf("Table '%s' does not exist in the database.\n", cur->tok_string);
		} else { 
			cur = cur->next; // expect 'VALUES'
			if ((cur != NULL) && (cur->tok_value != K_VALUES)) {
				// Error - keyword 'VALUES' is missing
				rc = INVALID_INSERTION_SYNTAX;
				cur->tok_value = INVALID;
				printf("Values keyword missing.\n", cur->tok_string);
			} else { 
				cur = cur->next; // expect '('
				if (cur->tok_value != S_LEFT_PAREN) {
					// Error - '(' was not found
					rc = INVALID_INSERTION_SYNTAX;
					cur->tok_value = INVALID;
					printf("Left parenthesis not found.\n", cur->tok_string);
				} else {
					memset(field_list, '\0', (MAX_NUM_COL * sizeof(field))); // zero out field list

					// Move to the first cd in tpd list using cd_offset 
					col_entry = (cd_entry*) ((char*) tpd_entry + tpd_entry->cd_offset);

					cur = cur->next; // check inserted values

					do {
						// string & int literals have tok_class == constant
						// null tok_class == keyword
						if ((cur->tok_class != constant) && (cur->tok_class != keyword)) {
							// Error - tok_class must be constant (string/int) or keyword (null)
							rc = INVALID_FIELD_VALUE;
							cur->tok_value = INVALID;
							printf("Field's tok_class must be constant (string/int) or keyword (null).\n");
						} else {
							// tok_value must be null, an int, or a string
							if (cur->tok_value != K_NULL &&
								cur->tok_value != INT_LITERAL &&
								cur->tok_value != STRING_LITERAL) {
								// Error - inserted value must be string, int, or null
								rc = INVALID_FIELD_VALUE;
								cur->tok_value = INVALID;
								printf("Field must be string, int, or null.\n");
							} else {
								if ((col_entry->not_null) &&
									(stricmp(cur->tok_string, "NULL") == 0) && 
									cur->tok_value == K_NULL) {
									// Error - not null enforced
									rc = NOT_NULL_ENFORCED;
									cur->tok_value = INVALID;
									printf("Column's not null constraint is enforced. Field cannot be null.\n");
								} else {
									if ((col_entry->col_type == T_INT && cur->tok_value == STRING_LITERAL) ||
										(col_entry->col_type == T_CHAR && cur->tok_value == INT_LITERAL)) {
										// Error - type mismatch; col_type does not match tok_value
										rc = TYPE_MISMATCH;
										cur->tok_value = INVALID;
										printf("Type mismatch. Column descriptor's col_type does not match tok_value.\n");
									} else if (col_entry->col_type == T_CHAR && // string
										stricmp(cur->tok_string, "NULL") != 0 && // not null
										strlen(cur->tok_string) > col_entry->col_len) { // string length against cd length
										// Error - string is longer than column length defined in column descriptor
										rc = STRING_TOO_LONG;
										cur->tok_value = INVALID;
										printf("Field string length is longer than specified length in column descriptor.\n");
									} else {
										// field is valid; add it to field_list
										field_list[field_count].value = cur->tok_string;
										field_list[field_count].type = col_entry->col_type;
										field_list[field_count].tok_value = cur->tok_value;
										field_list[field_count].length = col_entry->col_len;

										cur = cur->next; // should be a ',' or ')'
										if ((cur->tok_value != S_COMMA) && (cur->tok_value != S_RIGHT_PAREN)) {
											// Error - ',' or ')' was not found
											rc = INVALID_INSERTION_SYNTAX;
											cur->tok_value = INVALID;
											printf("Right parenthesis or comma not found.\n", cur->tok_string);
											break;
										} else {
											if (cur->tok_value == S_RIGHT_PAREN) {
												row_done = true;
											}
											cur = cur->next;
										}
									} // field is valid
								} // type mismatch
							} // not null enforced
						} // must be null, int, or string

						if (!rc) {
							field_count++;
							col_entry = (cd_entry*) ((char*) col_entry + sizeof(cd_entry)); // move to next column
						}
					} while ((rc == 0) && (!row_done));

					if (row_done) {
						if (cur->tok_value != EOC) {
						// Error - ')' was found when tok_list has not been terminated
						rc = INVALID_INSERTION_SYNTAX;
						cur->tok_value = INVALID;
						printf("Right parenthesis not found.\n");
						} else if (field_count != tpd_entry->num_columns) { // row done & # values does not match tpd # of cols
							rc = DIFFERENT_NUM_COLUMNS;
							cur->tok_value = INVALID;
							printf("# values inserted does not match # columns defined by tpd.\n");
						} else {
							// no errors; insert row into table
							rc = add_row_to_table(field_list, tpd_entry->table_name, tpd_entry->num_columns);
						}
					}
				} 
			}			
		}
	}
	return rc;
}

/*************************************************************
(6) Delete from table
*************************************************************/
int sem_delete_from(token_list *t_list) {
	int rc = 0; // return code (0 = no error)
	token_list *cur = t_list;
	tpd_entry *entry = NULL;
	condition *condition = NULL; 

	if ((cur != NULL) && 
		(cur->tok_class != keyword) &&
		(cur->tok_class != identifier) &&
		(cur->tok_class != type_name)) {
		printf("Invalid table name.\n");
		rc = INVALID_TABLE_NAME;
		cur->tok_class = INVALID;
	} else {
		if ((entry = get_tpd_from_list(cur->tok_string)) == NULL) {
			rc = TABLE_NOT_EXIST;
			cur->tok_value = INVALID;
			printf("Table '%s' does not exist in the database.", cur->tok_string);
		} else { // expect WHERE
			cur = cur->next;
			if (cur != NULL && cur->tok_value == EOC) { // delete all rows
				rc = delete_from_table(entry, NULL);
			} else { 
				if (cur != NULL && cur->tok_value == K_WHERE) {
					rc = sem_where(&cur, entry, &condition);
					if (!rc) {
						rc = delete_from_table(entry, condition);
						if (!rc) {
							free_conditions(condition);
						}
					}
				} else {
					rc = INVALID_DELETE_SYNTAX;
					cur->tok_value = INVALID;
					printf("Invalid delete statement.\n");
				}
			}
		}
	}
	return rc;
}

/*************************************************************
(7) Update table
*************************************************************/
int sem_update_table(token_list *t_list) {
	int rc = 0;
	token_list *cur = t_list;
	tpd_entry *tpd_entry = NULL;

	if ((cur != NULL) &&
		(cur->tok_class != keyword) &&
		(cur->tok_class != identifier) &&
		(cur->tok_class != type_name)) {
		rc = INVALID_TABLE_NAME;
		cur->tok_class = INVALID;
		printf("Invalid table name.\n");
	} else {
		if ((tpd_entry = get_tpd_from_list(cur->tok_string)) == NULL) {
			rc = TABLE_NOT_EXIST;
			cur->tok_value = INVALID;
			printf("Table '%s' does not exist in the database.", cur->tok_string);
		} else {
			cur = cur->next;
			if (cur != NULL && cur->tok_class != keyword && cur->tok_value != K_SET) {
				rc = INVALID_UPDATE_SYNTAX;
				cur->tok_value = INVALID;
				printf("Invalid update syntax. Set keyword expected & not found.\n");
			} else {
				cur = cur->next;
				if ((cur != NULL) &&
					(cur->tok_class != keyword) &&
					(cur->tok_class != identifier) &&
					(cur->tok_class != type_name)) {
					rc = INVALID_COLUMN_NAME;
					cur->tok_class = INVALID;
					printf("Invalid column name.\n");
				} else {
					int col_type = 0;
					rc = check_col_name(tpd_entry, cur->tok_string, &col_type, NULL, NULL);
					if (rc) {
						rc = COLUMN_NOT_EXIST;
						cur->tok_class = INVALID;
						printf("Column name does not exist.\n");
					} else {
						char *col_name = cur->tok_string;
						cur = cur->next;
						if (cur != NULL &&
							cur->tok_class != symbol &&
							cur->tok_value != S_EQUAL) {
							rc = INVALID_UPDATE_SYNTAX;
							cur->tok_class = INVALID;
							printf("Invalid update syntax.\n");
						} else {
							cur = cur->next;
							if (cur != NULL &&
								cur->tok_value != INT_LITERAL &&
								cur->tok_value != STRING_LITERAL &&
								cur->tok_value != K_NULL) {
								rc = INVALID_UPDATE_SYNTAX;
								cur->tok_value = INVALID;
								printf("Invalid update syntax. Value must be null, int, or string.\n");
							} else if ((cur != NULL) &&
								(col_type == T_INT && cur->tok_value != INT_LITERAL) &&
								(col_type == T_CHAR && cur->tok_value != STRING_LITERAL) &&
								(col_type == T_INT && cur->tok_value != K_NULL) &&
								(col_type == T_CHAR && cur->tok_value != K_NULL)) {
								printf("Type mismatch.\n");
								rc = TYPE_MISMATCH;
								cur->tok_value = INVALID;
							} else {
								rc = check_type_limits(cur, col_type);
								if (rc) {
									cur->tok_value = INVALID;
									printf("Data exceeds allowed limits.\n");
									return rc;
								}

								char *set_value = cur->tok_string; // value to set

								if (rc = check_value_against_cd_def(tpd_entry, col_name, cur)) {
									printf("Type mismatch or not null enforced.\n");
									return rc;
								}

								cur = cur->next;

								if (cur != NULL && cur->tok_value == EOC) {
									// update without where clause
									rc = update_table(tpd_entry, col_name, set_value, NULL);
								} else {
									if (cur != NULL && cur->tok_value == K_WHERE) { // update with where
										condition *cond;
										rc = sem_where(&cur, tpd_entry, &cond);
										if (!rc) {
											rc = update_table(tpd_entry, col_name, set_value, cond);
											if (!rc) {
												free_conditions(cond);
											}
										}
									} else {
										printf("Invalid update syntax.\n");
										rc = INVALID_UPDATE_SYNTAX;
										cur->tok_class = INVALID;
									}
								}
							}
						} 
					}
				}
			}
		}
	}

	return rc;
}

/*************************************************************
(8) Select 
*************************************************************/
int sem_select_from(token_list *t_list) {
	int rc = 0;
	token_list *cur = t_list;
	tpd_entry *tpd = NULL;

	if ((cur != NULL) &&
		(cur->tok_class != keyword) &&
		(cur->tok_class != identifier) &&
		(cur->tok_class != type_name) &&
		(cur->tok_class != symbol) &&
		(cur->tok_class != function_name)) {
		rc = INVALID_SELECT_SYNTAX;
		cur->tok_value = INVALID;
		printf("Invalid select syntax. Second token invalid for select.");
	} else {
		int select_type = 0, aggr_funct = 0;
		char *col_name;
		column_info *col_info_list = NULL;

		if (cur->tok_class == symbol && cur->tok_value == S_STAR) { // found *
			select_type = SELECT_STAR;
		} else if (cur->tok_class == function_name && 
			(cur->tok_value == F_SUM || cur->tok_value == F_AVG || cur->tok_value == F_COUNT)) { // found aggregate
			select_type = SELECT_AGGR_FUNCT;
			aggr_funct = cur->tok_value;
			cur = cur->next;
			if (cur->tok_value != S_LEFT_PAREN) { // after aggregate expect (
				rc = INVALID_SELECT_SYNTAX;
				cur->tok_value = INVALID;
				printf("Invalid select syntax. Expect ( after aggregate.\n");
			} else { 
				cur = cur->next;
				if (cur->tok_class == keyword || cur->tok_class == identifier ||
					cur->tok_class == type_name || cur->tok_value == S_STAR) {
					col_name = cur->tok_string;
					add_to_col_info_list(&col_info_list, col_name, 0, 0, 0);
					cur = cur->next;
					if (cur->tok_value != S_RIGHT_PAREN) {
						rc = INVALID_SELECT_SYNTAX;
						cur->tok_value = INVALID;
						printf("Invalid select syntax. Expect ).\n");
					}
				} else {
					rc = INVALID_COLUMN_NAME;
					cur->tok_value = INVALID;
					printf("Invalid column name during aggregation.\n");
				}
			}
		} else if (cur->tok_class == keyword || cur->tok_class == identifier || cur->tok_class == type_name) {
			select_type = SELECT_COL_NAME;
			bool col_done = false;
			int i = 0;
			token_list *end_col_list = NULL;

			do {
				if ((cur->tok_class != keyword) &&
					(cur->tok_class != identifier) &&
					(cur->tok_class != type_name)) {
					rc = INVALID_COLUMN_NAME;
					cur->tok_value = INVALID;
					printf("Invalid column name.\n");
				} else {
					i++;
					add_to_col_info_list(&col_info_list, cur->tok_string, 0, 0, 0);
					cur = cur->next;
					if ((cur->tok_value != S_COMMA) && (cur->tok_value != K_FROM)) {
						rc = INVALID_SELECT_SYNTAX;
						cur->tok_value = INVALID;
						printf("Invalid select syntax. Expecting comma or end column list.\n");
					} else {
						if (cur->tok_value == K_FROM) {
							col_done = true;
							end_col_list = cur;
						} else if (cur->tok_value == S_COMMA && (cur->next->tok_value == K_FROM || cur->next->tok_value == EOC)) {
							rc = INVALID_SELECT_SYNTAX;
							cur->tok_value = INVALID;
							printf("Invalid select syntax. Expecting end of column list or a comma.\n");
						}
						cur = cur->next;
					}
				}
			} while ((rc == 0) && (!col_done));

			if (!rc) {
				if (col_done || cur->tok_value == K_FROM) {
					cur = end_col_list;
				}
			}
		} else { // did not find *, aggregate, or col name
			printf("Invalid select syntax.\n");
			cur->tok_value = INVALID;
			rc = INVALID_SELECT_SYNTAX;
			return rc;
		}

		if (rc) { // report syntax errors found
			return rc;
		}

		if (select_type == SELECT_STAR || select_type == SELECT_AGGR_FUNCT) {
			cur = cur->next;
		}

		if ((cur->tok_class != keyword && cur->tok_value != K_FROM)) {
			rc = INVALID_SELECT_SYNTAX;
			cur->tok_value = INVALID;
			printf("Invalid select syntax. Expecting FROM.\n");
		} else {
			cur = cur->next;
			if ((cur->tok_class != keyword) &&
				(cur->tok_class != identifier) &&
				(cur->tok_class != type_name)) {
				rc = INVALID_TABLE_NAME;
				cur->tok_value = INVALID;
				printf("Invalid table name.\n");
			} else {
				tpd_entry *tpd_entry = NULL;

				if ((tpd_entry = get_tpd_from_list(cur->tok_string)) == NULL) {
					rc = TABLE_NOT_EXIST;
					cur->tok_value = INVALID;
					printf("Table '%s' does not exist in the database.", cur->tok_string);
				} else {
					if (select_type == SELECT_STAR || (aggr_funct == F_COUNT && strcmp(col_name, "*") == 0)) {
						int i, offset = 0, num_cols = tpd_entry->num_columns;
						cd_entry *col_entry = (cd_entry*) ((char*) tpd_entry + tpd_entry->cd_offset); // start at first cd

						for (i = 0; i < num_cols; i++) {
							add_to_col_info_list(&col_info_list, col_entry->col_name, col_entry->col_type, col_entry->col_len, offset);
							offset = offset + (1 + col_entry->col_len);
							col_entry = (cd_entry*) ((char*) col_entry + sizeof(cd_entry));
						}
					}

					if (select_type == SELECT_COL_NAME) {
						column_info *temp_c_list = col_info_list;
						while (temp_c_list != NULL) {
							rc = check_col_name(tpd_entry, temp_c_list->col_name, &(temp_c_list->col_type), &(temp_c_list->col_len), &(temp_c_list->col_offset));
							if (rc) {
								printf("Invalid column name.\n");
								return rc;
							}
							temp_c_list = temp_c_list->next;
						}
					}

					if (select_type == SELECT_AGGR_FUNCT) {
						if ((aggr_funct == F_AVG || aggr_funct == F_SUM) && strcmp(col_name, "*") == 0) {
							rc = INVALID_SELECT_SYNTAX;
							printf("AVG and SUM cannot take * as a column name.\n");
							return rc;
						} else if (aggr_funct == F_AVG || aggr_funct == F_SUM || 
							(aggr_funct == F_COUNT && strcmp(col_name, "*") != 0)) {
							rc = check_col_name(tpd_entry, col_name, &(col_info_list->col_type), &(col_info_list->col_len), &(col_info_list->col_offset));
							if (rc) {
								rc = INVALID_COLUMN_NAME;
								printf("Invalid column name in AVG, SUM or COUNT.\n");
								return rc;
							} else {
								if ((aggr_funct == F_SUM || aggr_funct == F_AVG) && (col_info_list->col_type == T_CHAR)) {
									rc = INVALID_TYPE_AGGREGATION;
									printf("AVG and SUM can only take in type INT.\n");
									return rc;
								}
							}
						}
					}

					cur = cur->next;
					if (cur->tok_value != EOC && cur->tok_value != K_WHERE && cur->tok_value != K_ORDER) {
						printf("Invalid select syntax. End of statement, order or where expected.\n");
						rc = INVALID_SELECT_SYNTAX;
						cur->tok_value = INVALID;
					} else if (cur->tok_value == EOC) {
						table_file_header *tfh_entry = get_records_from_tab_file(tpd_entry);
						char *table = (char*) ((char*) tfh_entry + tfh_entry->record_offset); // ptr to the first record
						int num_records = tfh_entry->num_records;
						int record_size = tfh_entry->record_size;

						if (num_records == 0) {
							printf("Warning: No row(s) found.\n");
							return rc;
						}
						if (table) {
							if (select_type == SELECT_STAR || select_type == SELECT_COL_NAME) {
								select(table, col_info_list, num_records, record_size);
							}
							else {
								select_aggr_funct(table, col_info_list, aggr_funct, col_name, num_records, record_size);
							}
						} else {
							rc = FILE_OPEN_ERROR;
							printf("Failed to open tab file.\n");
						}
					} else if (cur->tok_value == K_ORDER && cur->next != NULL && cur->next->tok_value == K_BY) { // order by
						if (select_type == SELECT_AGGR_FUNCT) {
							rc = INVALID_SELECT_SYNTAX;
							printf("Cannot order by when using aggregate functions.\n");
						} else {
							column_info *order_by_list = NULL;
							char *order_by_col_name = NULL;
							bool order_by_desc = false;
							rc = sem_order_by(tpd_entry, &cur, &order_by_list, &order_by_col_name, &order_by_desc);
							if (!rc) {
								table_file_header *tfh_entry = get_records_from_tab_file(tpd_entry);
								char *table = (char*) ((char*) tfh_entry + tfh_entry->record_offset); // ptr to the first record
								int num_records = tfh_entry->num_records;
								int record_size = tfh_entry->record_size;

								if (num_records == 0) {
									printf("Warning: No row(s) found.\n");
									return rc;
								}
								char *ordered_table = get_records_order(table, order_by_list, &num_records, &record_size, order_by_desc);
								if (ordered_table) {
									select(ordered_table, col_info_list, num_records, record_size);
								} else {
									rc = FILE_OPEN_ERROR;
								}
							} else {
								return rc;
							}
						}
					} else { // where
						condition *cond; 
						rc = sem_where(&cur, tpd_entry, &cond);

						if (!rc) {
							if (cur->tok_value == EOC) {
								table_file_header *tfh_entry = get_records_from_tab_file(tpd_entry);
								char *table = (char*) ((char*) tfh_entry + tfh_entry->record_offset); // ptr to the first record
								int num_records = tfh_entry->num_records;
								int record_size = tfh_entry->record_size;

								if (num_records == 0) {
									printf("Warning: No row(s) found.\n");
									return rc;
								}
								cd_entry *col_entry = (cd_entry*) ((char*) tpd_entry + tpd_entry->cd_offset);
								char *where_table = get_records_where(table, col_entry, cond, tpd_entry->num_columns, &num_records, &record_size);
								if (where_table) {
									if (select_type == SELECT_STAR || select_type == SELECT_COL_NAME) {
										select(where_table, col_info_list, num_records, record_size);
									}
									else {
										select_aggr_funct(where_table, col_info_list, aggr_funct, col_name, num_records, record_size);
									}
								} else {
									rc = FILE_OPEN_ERROR;
								}
							} else if (cur->tok_value == K_ORDER && cur->next != NULL && cur->next->tok_value == K_BY) {
								if (select_type == SELECT_AGGR_FUNCT) {
									rc = INVALID_SELECT_SYNTAX;
									printf("Cannot order by when using aggregate functions.\n");
								} else {
									column_info *order_by_list = NULL;
									char *order_by_col_name = NULL;
									bool order_by_desc = false;
									rc = sem_order_by(tpd_entry, &cur, &order_by_list, &order_by_col_name, &order_by_desc);
									if (!rc) {
										table_file_header *tfh_entry = get_records_from_tab_file(tpd_entry);
										char* table = (char*) ((char*) tfh_entry + tfh_entry->record_offset); // ptr to the first
										int num_records = tfh_entry->num_records;
										int record_size = tfh_entry->record_size;

										if (num_records == 0) {
											printf("Warning: No row(s) found.\n");
											return rc;
										}
										cd_entry *col_entry = (cd_entry*) ((char*) tpd_entry + tpd_entry->cd_offset);
										char *where_table = get_records_where(table, col_entry, cond, tpd_entry->num_columns, &num_records, &record_size);
										char *order_by_table = get_records_order(where_table, order_by_list, &num_records, &record_size, order_by_desc);
										if (order_by_table) {
											select(order_by_table, col_info_list, num_records, record_size);
										} else {
											rc = FILE_OPEN_ERROR;
										}
									} else {
										return rc;
									}
								}
							}
						}
					}
				}
			}
		} 
	}

	return rc;
}

/*************************************************************
(9) Back up
*************************************************************/
int sem_backup(token_list *t_list) {
	int rc = 0;
	token_list *cur = t_list;

	if ((cur->tok_class != keyword) &&
		(cur->tok_class != identifier) &&
		(cur->tok_class != type_name)) {
		rc = INVALID_IMAGE_FILE_NAME;
		cur->tok_value = INVALID;
		printf("Invalid image file name.\n");
	} else {
		if (cur->next->tok_value == EOC) {
			rc = backup_image(cur->tok_string);
			if (!rc) {
				// add BACKUP <image_name> to log
				memset(log_buffer, '\0', LOG_RECORD_LENGTH + 1);
				sprintf(log_buffer, "BACKUP %s\n", cur->tok_string);
				rc = append_to_log(log_buffer, NULL);
			} else {
				cur->tok_value = INVALID;
			}
		} else {
			printf("Invalid backup statement.\n");
			rc = INVALID_STATEMENT;
			cur->next->tok_value = INVALID;
		}
	}

	return rc;
}

/*************************************************************
(10) Restore
*************************************************************/
int sem_restore(token_list *t_list) {
	int rc = 0;
	token_list *cur = t_list, *temp;
	char *image_name;

	if (cur->tok_class != identifier && cur->tok_class != keyword && cur->tok_class != type_name) {
		printf("Invalid image file name.\n");
		rc = INVALID_IMAGE_FILE_NAME;
		cur->tok_value = INVALID;
	} else // valid image name
	{
		image_name = cur->tok_string;
		temp = cur;
		cur = cur->next;
		if (cur->tok_value == EOC) {
			if (!restore_image(image_name) && !toggle_dbflag()) {
				without_rf = false;
				append_to_log("RF_START\n", image_name);
			} else {
				printf("Invalid image file name.\n");
				rc = INVALID_IMAGE_FILE_NAME;
				temp->tok_value = INVALID;
			}
		} else if (cur->tok_value == K_WITHOUT) // without rf
		{
			cur = cur->next;
			if (cur->tok_value != K_RF) {
				printf("Invalid restore statement.\n");
				rc = INVALID_STATEMENT;
				cur->tok_value = INVALID;
			} else {
				cur = cur->next;
				if (cur->tok_value != EOC) {
					printf("Invalid restore statement.\n");
					rc = INVALID_STATEMENT;
					cur->tok_value = INVALID;
				} else {
					if (!restore_image(image_name)) {
						without_rf = true;
						append_to_log("RF_START\n", image_name);
					} else {
						printf("Unable to restore.\n");
						rc = COULD_NOT_RESTORE;
					}
				}
			}
		} else // invalid syntax
		{
			printf("Invalid restore statement syntax.\n");
			rc = INVALID_STATEMENT;
			cur->tok_value = INVALID;
		}
	}

	return rc;
}

/*************************************************************
(11) Rollforward
*************************************************************/
int sem_rollforward(token_list *t_list) {
	int rc = 0;
	token_list *cur = t_list;
	char *time_stamp;
	char *now;

	if (cur->tok_value == EOC) {
		time_stamp = NULL;
	} else if (cur->tok_value == K_TO) {
		cur = cur->next;
		if (cur->tok_value == INT_LITERAL && strlen(cur->tok_string) == TIME_STAMP_LENGTH) {
			time_stamp = cur->tok_string;
			now = make_time_stamp();
			if (strncmp(time_stamp, now, TIME_STAMP_LENGTH) > 0) {
				printf("Invalid timestamp. Cannot enter a future timestamp.\n");
				rc = INVALID_TIMESTAMP;
				cur->tok_value = INVALID;
			} else {
				cur = cur->next;
				if (cur->tok_value != EOC) {
					printf("Invalid rollforward syntax.\n");
					rc = INVALID_ROLLFORWARD_SYNTAX;
					cur->tok_value = INVALID;
				}
			}
		} else {
			rc = INVALID_TIMESTAMP;
			cur->tok_value = INVALID;
			printf("Invalid timestamp.\n");
		}
	} else {
		printf("Invalid rollforward syntax.\n");
		rc = INVALID_ROLLFORWARD_SYNTAX;
		cur->tok_value = INVALID;
	}

	if (!rc) {
		g_tpd_list->db_flags = 0;
		if ((rc = rollforward(time_stamp)) == 0) {
			g_tpd_list->db_flags = ROLLFORWARD_PENDING;
			rc = toggle_dbflag();
		}
	}

	return rc;
}

int initialize_tpd_list() {
	int rc = 0;
	FILE *fhandle = NULL;
	struct _stat file_stat;

	/* Open for read */
	if ((fhandle = fopen("dbfile.bin", "rbc")) == NULL) {
		if ((fhandle = fopen("dbfile.bin", "wbc")) == NULL) {
			rc = FILE_OPEN_ERROR;
		} else {
			g_tpd_list = NULL;
			g_tpd_list = (tpd_list*) calloc(1, sizeof(tpd_list));

			if (!g_tpd_list) {
				rc = MEMORY_ERROR;
			} else {
				g_tpd_list->list_size = sizeof(tpd_list);
				fwrite(g_tpd_list, sizeof(tpd_list), 1, fhandle);
				fflush(fhandle);
				fclose(fhandle);
			}
		}
	} else { 
		/* There is a valid dbfile.bin file - get file size */
		_fstat(_fileno(fhandle), &file_stat); 
		printf("dbfile.bin size = %d\n", file_stat.st_size);

		g_tpd_list = (tpd_list*) calloc(1, file_stat.st_size);

		if (!g_tpd_list) {
			rc = MEMORY_ERROR;
		} else {
			fread(g_tpd_list, file_stat.st_size, 1, fhandle);
			fflush(fhandle);
			fclose(fhandle);

			if (g_tpd_list->list_size != file_stat.st_size) {
				rc = DBFILE_CORRUPTION;
			}

		}
	}

	return rc;
}

int add_tpd_to_list(tpd_entry *tpd) {
	int rc = 0;
	int old_size = 0;
	FILE *fhandle = NULL;

	if ((fhandle = fopen("dbfile.bin", "wbc")) == NULL) {
		rc = FILE_OPEN_ERROR;
	} else {
		old_size = g_tpd_list->list_size;

		if (g_tpd_list->num_tables == 0) {
			/* If this is an empty list, overlap the dummy header */
			g_tpd_list->num_tables++;
			g_tpd_list->list_size += (tpd->tpd_size - sizeof(tpd_entry));
		} else {
			/* There is at least 1, just append at the end */
			g_tpd_list->num_tables++;
			g_tpd_list->list_size += tpd->tpd_size;
		}

		// add new table to g_tpd_list
		tpd_list *temp_g_tpd_list = (tpd_list*) calloc(1, g_tpd_list->list_size);
		if (temp_g_tpd_list == NULL) {
			printf("Failed to allocate memory for temporary g_tpd_list\n");
			rc = MEMORY_ERROR;
			return rc;
		}
		
		if (g_tpd_list->num_tables > 1) {
			memcpy(temp_g_tpd_list, g_tpd_list, old_size);
			memcpy((char *) temp_g_tpd_list + old_size, tpd, tpd->tpd_size);
		} else {
			memcpy(temp_g_tpd_list, g_tpd_list, old_size - sizeof(tpd_entry));
			memcpy((char *) temp_g_tpd_list + old_size - sizeof(tpd_entry), tpd, tpd->tpd_size);
		}
		
		free(g_tpd_list);

		g_tpd_list = temp_g_tpd_list;

		fwrite(g_tpd_list, g_tpd_list->list_size, 1, fhandle);
		fflush(fhandle);
		fclose(fhandle);
	}

	return rc;
}

int drop_tpd_from_list(char *tabname) {
	int rc = 0;
	tpd_entry *cur = &(g_tpd_list->tpd_start);
	int num_tables = g_tpd_list->num_tables;
	bool found = false;
	int count = 0;

	if (num_tables > 0) {
		while ((!found) && (num_tables-- > 0)) {
			if (stricmp(cur->table_name, tabname) == 0) {
				/* found it */
				found = true;
				int old_size = 0;
				FILE *fhandle = NULL;

				if ((fhandle = fopen("dbfile.bin", "wbc")) == NULL) {
					rc = FILE_OPEN_ERROR;
				} else {
					old_size = g_tpd_list->list_size;

					if (count == 0) {
						/* If this is the first entry */
						g_tpd_list->num_tables--;

						if (g_tpd_list->num_tables == 0) {
							/* This is the last table, null out dummy header */
							memset((void*) g_tpd_list, '\0', sizeof(tpd_list));
							g_tpd_list->list_size = sizeof(tpd_list);
							fwrite(g_tpd_list, sizeof(tpd_list), 1, fhandle);
						} else {
							/* First in list, but not the last one */
							g_tpd_list->list_size -= cur->tpd_size;

							/* First, write the 8 byte header */
							fwrite(g_tpd_list, sizeof(tpd_list) - sizeof(tpd_entry),
								1, fhandle);

							/* Now write everything starting after the cur entry */
							fwrite((char*) cur + cur->tpd_size,
								old_size - cur->tpd_size -
								(sizeof(tpd_list) - sizeof(tpd_entry)),
								1, fhandle);
						}
					} else {
						/* This is NOT the first entry - count > 0 */
						g_tpd_list->num_tables--;
						g_tpd_list->list_size -= cur->tpd_size;

						/* First, write everything from beginning to cur */
						fwrite(g_tpd_list, ((char*) cur - (char*) g_tpd_list),
							1, fhandle);

						/* Check if cur is the last entry. Note that g_tdp_list->list_size
							 has already subtracted the cur->tpd_size, therefore it will
							 point to the start of cur if cur was the last entry */
						if ((char*) g_tpd_list + g_tpd_list->list_size == (char*) cur) {
							/* If true, nothing else to write */
						} else {
							/* NOT the last entry, copy everything from the beginning of the
								 next entry which is (cur + cur->tpd_size) and the remaining size */
							fwrite((char*) cur + cur->tpd_size,
								old_size - cur->tpd_size -
								((char*) cur - (char*) g_tpd_list),
								1, fhandle);
						}
					}

					fflush(fhandle);
					fclose(fhandle);
				}


			} else {
				if (num_tables > 0) {
					cur = (tpd_entry*) ((char*) cur + cur->tpd_size);
					count++;
				}
			}
		}
	}

	if (!found) {
		rc = INVALID_TABLE_NAME;
	}

	return rc;
}

tpd_entry* get_tpd_from_list(char *tabname) {
	tpd_entry *tpd = NULL;
	tpd_entry *cur = &(g_tpd_list->tpd_start);
	int num_tables = g_tpd_list->num_tables;
	bool found = false;

	if (num_tables > 0) {
		while ((!found) && (num_tables-- > 0)) {
			if (stricmp(cur->table_name, tabname) == 0) {
				/* found it */
				found = true;
				tpd = cur;
			} else {
				if (num_tables > 0) {
					cur = (tpd_entry*) ((char*) cur + cur->tpd_size);
				}
			}
		}
	}

	return tpd;
}

/*************************************************************
ADDED FUNCTIONS BELOW
*************************************************************/

/* Create <tablename>.tab file after successful create table statement */
int create_table_file(char *table_name, int record_size) {
	int rc = 0;
	char tf_name[MAX_IDENT_LEN + 5]; // MAX_IDENT_LEN + ".tab" (4 bytes) + null terminator (1 byte)
	FILE *fhandle;
	table_file_header tfh;

	// if it isn't already, make record_size multiple of 4 
	if (record_size % 4 != 0) {
		record_size += (4 - (record_size % 4));
	}

	memset(&tfh, '\0', sizeof(table_file_header)); // 0 out table structure

	add_tab_extension(tf_name, table_name);

	if ((fhandle = fopen(tf_name, "wb")) == NULL) {
		rc = FILE_OPEN_ERROR;
		printf("Failed to open %s for write", tf_name);
	} else {
		tfh.file_size = sizeof(table_file_header);
		tfh.record_size = record_size;
		tfh.num_records = 0; // initially 0 records
		tfh.record_offset = sizeof(table_file_header);
		fwrite(&tfh, sizeof(table_file_header), 1, fhandle);
		fclose(fhandle);
	}
	return rc;
}

/* After sem checking for insert into, add the row to .tab */
int add_row_to_table(field *field_list, char *table_name, int num_cols) {
	int rc = 0;
	char tf_name[MAX_IDENT_LEN + 5]; // MAX_IDENT_LEN + ".tab" (4 bytes) + null terminator (1 byte)
	FILE *fhandle;
	table_file_header existing_tfh; // tfh read from .tab file
	table_file_header *new_tf;
	struct _stat fstat;

	add_tab_extension(tf_name, table_name);

	if ((fhandle = fopen(tf_name, "rb")) == NULL) {
		rc = FILE_OPEN_ERROR;
		printf("Failed to open %s for read", tf_name);
	} else {
		_fstat(_fileno(fhandle), &fstat); // valid .tab file exists

		memset(&existing_tfh, '\0', sizeof(table_file_header)); // zero out for read

		fread(&existing_tfh, sizeof(table_file_header), 1, fhandle); // read file contents into read_tfh

																		// allocate memory for existing file contents + one more record
		new_tf = (table_file_header*) calloc(1, existing_tfh.file_size + existing_tfh.record_size);

		if (!new_tf) { // failed to calloc
			rc = MEMORY_ERROR;
			printf("Failed to calloc tf.");
		} else {
			if (existing_tfh.file_size != fstat.st_size) {
				rc = CORRUPTED_TABFILE;
				printf("%s is corrupted. File size defined by tfh does not match actual file size.", tf_name);
			} else {
				fseek(fhandle, 0, SEEK_SET); // move fhandle back to beginning of file
				fread(new_tf, existing_tfh.file_size, 1, fhandle); // read contents of file to new_tf
				fclose(fhandle); // close fhandle for reading

				if (new_tf->num_records == 100) {
					rc = EXCEEDED_MAX_ROWS;
					printf("System only supports 100 rows in each table.");
				} else {
					new_tf->file_size += new_tf->record_size; // + record_size to file size
					new_tf->num_records++; // increment number of records

					char *temp = (char*) ((char*) new_tf + existing_tfh.file_size); // move to the last row
					int int_len = sizeof(int), null = 0, int_value, string_len;
					int field_count = 0;

					// add the new record to the end of the block
					while (field_count < num_cols) {
						if (field_list[field_count].type == T_INT) // integer
						{
							if (stricmp(field_list[field_count].value, "NULL") || field_list[field_count].tok_value != K_NULL) {
								memcpy(temp, &int_len, 1); // set first byte of the row
								temp = temp + 1;
								int_value = atoi(field_list[field_count].value);
								memcpy(temp, &int_value, sizeof(int));
							} else // NULL
							{
								memcpy(temp, &null, 1);
								temp = temp + 1;
								memcpy(temp, &null, sizeof(int));
							}
							temp = temp + sizeof(int); // move to the next column
						} else // string
						{
							if (stricmp(field_list[field_count].value, "NULL") || field_list[field_count].tok_value != K_NULL) {
								string_len = strlen(field_list[field_count].value);
								memcpy(temp, &string_len, 1);
								temp = temp + 1;
								memcpy(temp, field_list[field_count].value, string_len);
							} else // NULL
							{
								memcpy(temp, &null, 1);
								temp = temp + 1;
							}

							temp = temp + field_list[field_count].length;
						}

						field_count++;
					}

					if ((fhandle = fopen(tf_name, "wb")) == NULL) {
						rc = FILE_OPEN_ERROR;
					} else {
						fwrite(new_tf, new_tf->file_size, 1, fhandle);

						if (!rc) {
							printf("%s new size = %d\n", tf_name, new_tf->file_size);
						}

						fclose(fhandle);
					}
				}
			}
		}
		free(new_tf);
	}


	return rc;
}

/* Helper is used to display a table */
void select(char *table, column_info *col_info_list, int num_records, int record_size) {
	int rc = 0;
	FILE *fhandle;
	struct _stat fstat;
	char tf_name[MAX_IDENT_LEN + 5];
	column_info *col_info_ptr;

	int i, j, k; // for iteration

	for (i = 0; i < num_records + 4; i++) {
		// first char of every row
		if (i == 0 || i == 2 || (i == num_records + 3)) {
			printf("+");
		} else {
			printf("|");
		}

		col_info_ptr = col_info_list;
		// in between first and last chars
		while (col_info_ptr != NULL) { // iterate all columns
			int col_len = col_info_ptr->col_len; // the length of the column 
			int ui_col_len; // length of content
			if (col_info_ptr->col_type == T_CHAR) {
				ui_col_len = max(col_len, strlen(col_info_ptr->col_name));
			} else {
				ui_col_len = 11;
			}

			if (i == 0 || i == 2 || (i == num_records + 3)) {
				for (k = 0; k < ui_col_len; k++) { // print '-' for length of col
					printf("-"); // in between
				}
				printf("+");
			} else if (i == 1) {
				printf("%-*s|", ui_col_len, col_info_ptr->col_name);
			} else {
				char *data_ptr = table + col_info_ptr->col_offset;
				int actual_len = 0; // actual length of the field
				char char_field[MAX_STRING_LEN];
				int int_field;
				memcpy(&actual_len, data_ptr, 1); // copy first byte of field to actual_len

				if (col_info_ptr->col_type == T_CHAR) {
					memset(char_field, '\0', MAX_STRING_LEN);
					memcpy(char_field, data_ptr + 1, col_len);
					if (actual_len > 0) {
						printf("%-*s|", ui_col_len, char_field);
					} else {
						printf("%-*s|", ui_col_len, "-");
					}
				} else {
					memcpy(&int_field, data_ptr + 1, sizeof(int));
					if (actual_len > 0) {
						printf("%*d|", ui_col_len, int_field);
					} else {
						printf("%*s|", ui_col_len, "-");
					}
				}
			}
			col_info_ptr = col_info_ptr->next;
		}

		if (i != 0 && i != 1 && i != 2 && (i != num_records + 3)) {
			table += record_size;
		}
			
		printf("\n");
	}
	printf("%d row(s) selected.\n", num_records);
}

/* Helper is used to display an aggregate table */
void select_aggr_funct(char *table, column_info *col_info_list, int aggr_funct, char *col_name, int num_records, int record_size) {
	char ui_col_name[MAX_IDENT_LEN + 8];
	memset(ui_col_name, '\0', MAX_IDENT_LEN + 8); 
	int sum = 0, count_nulls = 0;

	if (aggr_funct == F_COUNT) {
		strcpy(ui_col_name, "COUNT");
	} else if (aggr_funct == F_AVG) {
		strcpy(ui_col_name, "AVG");
	} else {
		strcpy(ui_col_name, "SUM");
	}

	strcat(ui_col_name, "(");
	strcat(ui_col_name, col_name);
	strcat(ui_col_name, ")");
	int ui_col_len = max(12, strlen(ui_col_name));

	if (aggr_funct == F_AVG || aggr_funct == F_SUM ||
		(aggr_funct == F_COUNT && strcmp(col_name, "*") != 0)) {
		for (int i = 0; i < num_records; i++) {
			int temp_length = 0, temp_int = 0;
			memcpy(&temp_length, table + col_info_list->col_offset, 1);
			if (temp_length > 0) {
				memcpy(&temp_int, table + col_info_list->col_offset + 1, sizeof(int));
				sum += temp_int;
			} else {
				count_nulls++;
			}
			table = table + record_size;
		}
	}

	if (num_records == 0) {
		printf("Warning: No row(s) found.\n");
		return;
	}

	for (int i = 0; i < 5; i++) {
		if (i == 0 || i == 2 || i == 4) {
			printf("+");
			for (int j = 0; j < ui_col_len; j++) printf("-");
			printf("+");
		} else if (i == 1) {
			printf("|%-*s|", ui_col_len, ui_col_name);
		} else {
			if (aggr_funct == F_COUNT && strcmp(col_name, "*") == 0) {
				printf("|%*d|", ui_col_len, num_records);
			} else if (aggr_funct == F_COUNT && strcmp(col_name, "*") != 0) {
				printf("|%*d|", ui_col_len, num_records - count_nulls);
			} else if (aggr_funct == F_AVG) {
				printf("|%*.2f|", ui_col_len, ((double) sum) / (num_records - count_nulls));
			} else {
				printf("|%*d|", ui_col_len, sum);
			}
		}
		printf("\n");
	}
	printf("1 row(s) selected.\n");
}

/* Delete from a table */
int delete_from_table(tpd_entry *tpd_entry, condition *cond) {
	int rc = 0, count = 0;
	FILE *fhandle = NULL; 
	struct _stat file_stat;
	table_file_header *tfh_entry;
	cd_entry *col_entry;
	char table_fname[MAX_IDENT_LEN + 5]; 

	add_tab_extension(table_fname, tpd_entry->table_name);

	if ((fhandle = fopen(table_fname, "rb")) == NULL) {
		rc = FILE_OPEN_ERROR;
	} else {
		_fstat(_fileno(fhandle), &file_stat);
		tfh_entry = (table_file_header*) calloc(1, file_stat.st_size);

		if (!tfh_entry) {
			rc = MEMORY_ERROR;
			printf("Failed to calloc for temporary tfh_entry.\n");
		} else {
			fread(tfh_entry, file_stat.st_size, 1, fhandle);
			fclose(fhandle);

			if (tfh_entry->num_records == 0) {
				printf("Warning: No rows were found.\n");
				return rc;
			}

			if (cond == NULL) { // no conditions
				printf("%d row(s) deleted.\n", tfh_entry->num_records);
				tfh_entry->num_records = 0;
				tfh_entry->file_size = sizeof(table_file_header);
			} else {
				int num_columns = tpd_entry->num_columns;
				char *first_record = (char*) ((char*) tfh_entry + tfh_entry->record_offset); // ptr to the first record
				char *last_record = (char*) ((char*) tfh_entry + tfh_entry->file_size - tfh_entry->record_size); // ptr to the last record

				while (first_record != (last_record + tfh_entry->record_size)) {
					// reset cd_entry to the first column descriptor
					col_entry = (cd_entry*) ((char*) tpd_entry + tpd_entry->cd_offset);

					if (check_condition(col_entry, num_columns, first_record, cond)) {
						count++;
						memcpy(first_record, last_record, tfh_entry->record_size);
						last_record = last_record - tfh_entry->record_size; // move last_record back up one record
					} else {
						first_record = first_record + tfh_entry->record_size;
					}
				}

				if (count == 0) {
					printf("Warning: No rows were found.\n");
				} else {
					tfh_entry->num_records = tfh_entry->num_records - count;
					tfh_entry->file_size = tfh_entry->file_size - (count * tfh_entry->record_size);
				}
			}

			if ((fhandle = fopen(table_fname, "wb")) == NULL) {
				rc = FILE_OPEN_ERROR;
			} else {
				fwrite(tfh_entry, tfh_entry->file_size, 1, fhandle);
				fclose(fhandle);
				if (count) {
					printf("%d row(s) deleted.\n", count);
				}
			}

			free(tfh_entry);
		}
	}

	return rc;
}

/* Update the table */
int update_table(tpd_entry *tpd_ptr, char* col_name, char *set_value, condition *cond) {
	int rc = 0, count = 0;
	FILE *fhandle = NULL;
	struct _stat file_stat;
	table_file_header *tfh_entry;
	char table_fname[MAX_IDENT_LEN + 5]; 

	add_tab_extension(table_fname, tpd_ptr->table_name);

	if ((fhandle = fopen(table_fname, "r+b")) == NULL) {
		rc = FILE_OPEN_ERROR;
	} else {
		_fstat(_fileno(fhandle), &file_stat);
		tfh_entry = (table_file_header*) calloc(1, file_stat.st_size);

		if (!tfh_entry) {
			rc = MEMORY_ERROR;
			printf("Failed to calloc for temporary tfh_entry.\n");
		} else {
			fread(tfh_entry, file_stat.st_size, 1, fhandle);
			fclose(fhandle);

			if (tfh_entry->num_records == 0) {
				printf("Warning: No rows found.\n");
				return rc;
			}

			char *first_record = (char*) ((char*) tfh_entry + tfh_entry->record_offset); // ptr to the first record
			char *last_record = (char*) ((char*) tfh_entry + tfh_entry->file_size - tfh_entry->record_size); // ptr to the last record

			if (cond == NULL) { // update all
				rc = update_helper(first_record, last_record, tfh_entry, tpd_ptr, col_name, set_value, cond, &count);
				if (rc) {
					return rc;
				}
				count = tfh_entry->num_records;
			} else {
				rc = update_helper(first_record, last_record, tfh_entry, tpd_ptr, col_name, set_value, cond, &count);
				if (rc) {
					return rc;
				}
			}

			if (!rc) {
				if ((fhandle = fopen(table_fname, "wb")) == NULL) {
					rc = FILE_OPEN_ERROR;
					printf("Failed to open tab file for writing.\n");
				} else {
					fwrite(tfh_entry, tfh_entry->file_size, 1, fhandle);
					fclose(fhandle);
					if (count) {
						printf("%d row(s) affected.\n", count);
					} else {
						printf("Warning: No row(s) affected.\n");
					}
				}
			}
			free(tfh_entry);
		}
	}

	return rc;
}

/* Backup an image of the current db */
int backup_image(char *image_name) {
	int rc = 0;
	FILE *fhandle;
	tpd_entry *tpd = &(g_tpd_list->tpd_start);
	table_file_header *tfh_entry;
	int num_tables = g_tpd_list->num_tables, i;

	if ((fhandle = fopen(image_name, "rb")) != NULL) {
		rc = IMAGE_FILE_ALREADY_EXISTS;
		printf("Image file %s already exists in the directory.\n", image_name);
		fclose(fhandle);
	} else if ((fhandle = fopen(image_name, "wb")) == NULL) {
		rc = FILE_OPEN_ERROR;
	} else {
		fwrite(g_tpd_list, g_tpd_list->list_size, 1, fhandle);
		i = 0;
		while (i < num_tables) {
			if ((tfh_entry = get_records_from_tab_file(tpd)) == NULL) {
				rc = FILE_OPEN_ERROR;
				printf("Failed to retrieve tpd and records.\n", tpd->table_name);
				fclose(fhandle);
				remove(image_name);
				break;
			} else {
				fwrite(&(tfh_entry->file_size), sizeof(int), 1, fhandle);
				fwrite(tfh_entry, tfh_entry->file_size, 1, fhandle);
				free(tfh_entry);
				tpd = (tpd_entry*) ((char*) tpd + tpd->tpd_size);
				i++;
			}
		}
		fclose(fhandle);
		if (!rc) {
			printf("%d table(s) backed up.\n", num_tables);
		}
	}

	return rc;
}

/* Restore db to image file state */
int restore_image(char *image_name) {
	int i, rc = 0, num_tables = 0, table_size, dbfile_size = 0;
	tpd_entry *new_entry;
	FILE *backup, *fhandle;
	char table_name[MAX_IDENT_LEN + 5];
	table_file_header *tfh_entry;
	tpd_list temp_tpd;

	memset(&temp_tpd, '\0', sizeof(tpd_list));

	if ((backup = fopen(image_name, "rb")) == NULL) {
		rc = FILE_OPEN_ERROR;
	} else {
		fread(&temp_tpd, sizeof(tpd_list), 1, backup); 
		dbfile_size = temp_tpd.list_size;
		fseek(backup, 0, SEEK_SET); 
		if (g_tpd_list != NULL) free(g_tpd_list);
		g_tpd_list = (tpd_list*) calloc(1, dbfile_size);
		if (!g_tpd_list) {
			printf("Unable to reserve memory for tpd list\n");
			rc = MEMORY_ERROR;
			return rc;
		}
		fread(g_tpd_list, dbfile_size, 1, backup);
		if ((fhandle = fopen("dbfile.bin", "wb")) == NULL) {
			rc = FILE_OPEN_ERROR;
		} else {
			fwrite(g_tpd_list, dbfile_size, 1, fhandle);
			fclose(fhandle);
		}

		if (!rc) {
			num_tables = g_tpd_list->num_tables;
			for (i = 0, new_entry = &(g_tpd_list->tpd_start); i < num_tables;
				i++, new_entry = (tpd_entry*) ((char*) new_entry + new_entry->tpd_size)) {
				table_size = 0;
				fread(&table_size, sizeof(int), 1, backup); // read size of table

				add_tab_extension(table_name, new_entry->table_name);

				tfh_entry = (table_file_header*) calloc(1, table_size);
				fread(tfh_entry, table_size, 1, backup);

				if ((rc = restore_tab_file(tfh_entry, table_name)) != 0) {
					break;
				}
				free(tfh_entry);
			}
		}
		printf("%d table(s) restored.\n", num_tables);
		fclose(backup);
	}

	return rc;
}

/* Rollforward to timestamp if not null. If null, rollforward all */
int rollforward(char *time_stamp) {
	int rc = 0, counter = 0;
	FILE *fhandle, *temp_fhandle;
	char buffer[LOG_RECORD_LENGTH];
	char *temp_query;

	if ((fhandle = fopen(LOG_FILE_NAME, "r")) == NULL) {
		printf("Failed to open log file.\n");
		rc = FILE_OPEN_ERROR;
	} else {
		if ((temp_fhandle = fopen(TEMP_LOG_FILE, "w")) == NULL) {
			rc = FILE_OPEN_ERROR;
			printf("Failed to open a temporary log file.\n");
		} else {
			while (fgets(buffer, LOG_RECORD_LENGTH, fhandle)) {
				if (strncmp(buffer, "RF_START", 8) != 0) {
					fprintf(temp_fhandle, buffer);
				} else {
					break;
				}
			}

			while (fgets(buffer, LOG_RECORD_LENGTH, fhandle)) {
				if (time_stamp != NULL) {
					if (strncmp(buffer, time_stamp, TIME_STAMP_LENGTH) > 0) {
						if (counter == 0) {
							rc = INVALID_TIMESTAMP;
							printf("ERROR: Timestamp provided is before any entry after RF_START\n");
						}
						break;
					}
				}
				fprintf(temp_fhandle, buffer);
				if ((temp_query = strstr(buffer, "\""))) {
					temp_query++;
					*(temp_query + strlen(temp_query) - 2) = '\0';
					if ((rc = redo_logged(temp_query)) == 0) {
						counter++;
					}
				}
			}
			fclose(temp_fhandle);
		}
		fclose(fhandle);
	}

	if (!rc) {
		if (time_stamp) {
			rename(LOG_FILE_NAME, get_backup_log_name());
		} else {
			remove(LOG_FILE_NAME);
		}
		rename(TEMP_LOG_FILE, LOG_FILE_NAME);
		printf("%d query/queries executed.\n", counter);
	}

	return rc;
}

/* Update the dbflag */
int toggle_dbflag() {
	int rc = 0;
	FILE *fhandle;

	if (g_tpd_list->db_flags == 0) {
		g_tpd_list->db_flags = ROLLFORWARD_PENDING;
	} else {
		g_tpd_list->db_flags = 0;
	}

	if ((fhandle = fopen("dbfile.bin", "wb")) == NULL) {
		rc = FILE_OPEN_ERROR;
		printf("Failed to open dbfile.bin.\n");
	} else {
		fwrite(g_tpd_list, g_tpd_list->list_size, 1, fhandle);
		fclose(fhandle);
	}

	return rc;
}

/* Restore a tab file */
int restore_tab_file(table_file_header *tfh_entry, char *table_name) {
	int rc = 0;
	FILE *fhandle;

	if ((fhandle = fopen(table_name, "wb")) == NULL) {
		rc = FILE_OPEN_ERROR;
		printf("Failed to open tab file for write.\n");
	} else {
		fwrite(tfh_entry, tfh_entry->file_size, 1, fhandle);
		fclose(fhandle);
	}

	return rc;
}

/* Redo all the logged transactions after RF_START */
int redo_logged(char *query) {
	int rc = 0;
	token_list *tok_list = NULL, *tok_ptr, *tmp_tok_ptr;

	if ((rc = get_token(query, &tok_list)) == 0) {
		rc = do_semantic(tok_list);
	}

	tok_ptr = tok_list;
	while (tok_ptr != NULL) {
		tmp_tok_ptr = tok_ptr->next;
		free(tok_ptr);
		tok_ptr = tmp_tok_ptr;
	}

	return rc;
}

/* Get a file name for backup log. db.log1, db.log2 ... */
char* get_backup_log_name() {
	FILE *fhandle = NULL;
	int i = 1;
	char temp_fname[MAX_IDENT_LEN + 5];
	char *fname = (char*) calloc(1, MAX_IDENT_LEN + 5);
	bool found = false;

	// increment appending number until not found
	while (!found) {
		memset(fname, '\0', MAX_IDENT_LEN + 5);
		strcpy(fname, LOG_FILE_NAME);
		itoa(i, temp_fname, 10);
		strcat(fname, temp_fname);

		if ((fhandle = fopen(fname, "r")) == NULL) {
			found = true;
		}
		i++;
	}

	return fname;
}

/* Format a time stamp */
char *make_time_stamp() {
	time_t timer;
	struct tm *time_info;

	time(&timer);
	time_info = localtime(&timer);
	strftime(time_buffer, TIME_STAMP_LENGTH + 1, "%Y%m%d%H%M%S", time_info);

	return time_buffer;
}

/* Append an entry to a log file.*/
int append_to_log(char *entry, char *image_name) {
	int rc = 0;
	FILE *fhandle, *temp_fhandle;
	char *temp = (char*) calloc(1, strlen(entry) + 1);
	strcpy(temp, entry);
	char *first_token = strtok(temp, " \n");
	bool pruned;

	if (stricmp(first_token, "RF_START") == 0) {
		if ((fhandle = fopen(LOG_FILE_NAME, "r")) == NULL) {
			rc = FILE_OPEN_ERROR;
			printf("Failed to open log file.\n");
		} else {
			if ((temp_fhandle = fopen(TEMP_LOG_FILE, "w")) == NULL) {
				rc = FILE_OPEN_ERROR;
				printf("Failed to open log file.\n");
			} else {
				char buffer[LOG_RECORD_LENGTH];

				while (fgets(buffer, LOG_RECORD_LENGTH, fhandle)) {
					fprintf(temp_fhandle, buffer);
					if ((strstr(buffer, "BACKUP") == buffer) && strstr(buffer, image_name) && !without_rf) {
						fprintf(temp_fhandle, entry);
					} else if ((strstr(buffer, "BACKUP") == buffer) && strstr(buffer, image_name) && without_rf) {
						if (feof(fhandle)) {
							pruned = false;
						} else {
							pruned = true;
						}
						break;
					}
				}
				fclose(fhandle);
				fclose(temp_fhandle);

				if (!without_rf) {
					// remove old log and make temp log the new log
					if (remove(LOG_FILE_NAME) != 0 || rename(TEMP_LOG_FILE, LOG_FILE_NAME) != 0) {
						printf("Failed to write to log file.\n");
						rc = WRITE_TO_LOG_ERROR;
					}
				} else {
					if (!pruned) {
						remove(TEMP_LOG_FILE);
					} else {
						if (rename(LOG_FILE_NAME, get_backup_log_name()) != 0 ||
							rename(TEMP_LOG_FILE, LOG_FILE_NAME) != 0) {
							rc = WRITE_TO_LOG_ERROR;
							printf("Failed to write to log file.\n");
						}
					}
				}
			}
		}
	} else {
		if ((fhandle = fopen(LOG_FILE_NAME, "a")) == NULL) {
			rc = FILE_OPEN_ERROR;
			printf("Failed to open log file.\n");
		} else {
			if (stricmp(first_token, "BACKUP") == 0) {
				fprintf(fhandle, entry);
			} else {
				fprintf(fhandle, "%s \"%s\"\n", make_time_stamp(), entry);
			}
			fclose(fhandle);
		}
	}
	free(temp);

	return rc;
}

/* Get records that satisfy where */
char *get_records_where(char *table, cd_entry *col_entry, condition *cond, int num_cols, int *num_records, int *record_size) {
	int num_recs = *num_records, rec_size = *record_size;
	int new_num_recs = 0;

	char *temp_table = (char*) calloc(1, num_recs * rec_size);
	char *ptr_temp_table = temp_table;
	char *first_record = table;
	char *last_record = table + (num_recs - 1) * rec_size;
	cd_entry *temp_cd;

	while (first_record != (last_record + rec_size)) {
		temp_cd = col_entry;
		if (check_condition(temp_cd, num_cols, first_record, cond)) {
			new_num_recs++;
			memcpy(ptr_temp_table, first_record, rec_size);
			ptr_temp_table = ptr_temp_table + rec_size;
		}
		first_record = first_record + rec_size;
	}

	*num_records = new_num_recs;

	return temp_table;
}

/* Comparator for descending order */
int compare_desc(const void* rec1, const void* rec2) {
	int len1 = 0, len2 = 0;
	int temp_int1 = 0, temp_int2 = 0;

	memcpy(&len1, (char*) rec1 + offset_for_qsort, 1);
	memcpy(&len2, (char*) rec2 + offset_for_qsort, 1);
	if (type_for_qsort == T_INT) {
		memcpy(&temp_int1, (char*) rec1 + offset_for_qsort + 1, sizeof(int));
		memcpy(&temp_int2, (char*) rec2 + offset_for_qsort + 1, sizeof(int));
		if (len1 < len2) return -1;
		else if (len1 > len2) return 1;
		else {
			return temp_int2 - temp_int1;
		}
	} else {
		if (len1 == 0 && len2 != 0) return -1;
		else if (len1 != 0 && len2 == 0) return 1;
		else if (len1 != 0 && len2 != 0) {
			return -(strncmp((char*) rec1 + offset_for_qsort + 1,
				(char*) rec2 + offset_for_qsort + 1,
				max(len1, len2)));
		} else {
			return 0;
		}
	}
}

/* Comparator for ascending order */
int compare_asc(const void* rec1, const void* rec2) {
	int len1 = 0, len2 = 0;
	int temp_int1 = 0, temp_int2 = 0;

	memcpy(&len1, (char*) rec1 + offset_for_qsort, 1);
	memcpy(&len2, (char*) rec2 + offset_for_qsort, 1);
	if (type_for_qsort == T_INT) {
		memcpy(&temp_int1, (char*) rec1 + offset_for_qsort + 1, sizeof(int));
		memcpy(&temp_int2, (char*) rec2 + offset_for_qsort + 1, sizeof(int));
		if (len1 < len2) return 1;
		else if (len1 > len2) return -1;
		else {
			return temp_int1 - temp_int2;
		}
	} else {
		if (len1 == 0 && len2 != 0) return 1;
		else if (len1 != 0 && len2 == 0) return -1;
		else if (len1 != 0 && len2 != 0) {
			return strncmp((char*) rec1 + offset_for_qsort + 1,
				(char*) rec2 + offset_for_qsort + 1,
				max(len1, len2));
		} else {
			return 0;
		}
	}
}

/* Get records that satisfy order by */
char *get_records_order(char *table, column_info *order_by_list, int *num_records, int *record_size, bool desc) {
	type_for_qsort = order_by_list->col_type;
	offset_for_qsort = order_by_list->col_offset;
	sort_table(table, *num_records, *record_size, desc);

	return table;
}

/* Sorts the table using quick sort */
void sort_table(char *table, int num_records, int record_size, bool desc) {
	if (desc) {
		qsort(table, num_records, record_size, compare_desc);
	} else {
		qsort(table, num_records, record_size, compare_asc);
	}
}

/* Add a column to col_info list for printing */
void add_to_col_info_list(column_info **clist, char *col_name, int type, int length, int offset) {
	column_info *cur = *clist;
	column_info *ptr = NULL;

	ptr = (column_info*) calloc(1, sizeof(column_info));
	ptr->col_name = col_name;
	ptr->col_type = type;
	ptr->col_len = length;
	ptr->col_offset = offset;

	// add to end of list
	if (cur == NULL) {
		*clist = ptr;
	} else {
		while (cur->next != NULL) {
			cur = cur->next;
		}
		cur->next = ptr;
	}
}

/* A helper method for updating fields */
int update_helper(char *first_record, char *last_record, table_file_header *tfh_entry,
	tpd_entry *tpd_ptr, char *col_name, char *set_value, condition *cond, int *count) {
	int rc = 0;
	int i, num_cols = tpd_ptr->num_columns;
	cd_entry *col_entry, *temp_cd;
	char *temp, *temp_first;

	while (first_record != (last_record + tfh_entry->record_size)) {
		temp = first_record;
		temp_first = first_record;
		col_entry = (cd_entry*) ((char*) tpd_ptr + tpd_ptr->cd_offset);
		for (i = 0; i < num_cols; i++) {
			if (strcmp(col_entry->col_name, col_name) == 0) {
				if (cond == NULL) { // no conditions
					rc = set_column(&temp, col_entry, set_value);
				} else { // with where
					temp_cd = (cd_entry*) ((char*) tpd_ptr + tpd_ptr->cd_offset);
					if (check_condition(temp_cd, num_cols, temp_first, cond)) {
						*count = *count + 1;
						rc = set_column(&temp, col_entry, set_value);
					}
				}
			} else {
				temp += 1 + col_entry->col_len;
			}
			col_entry = (cd_entry*) ((char*) col_entry + sizeof(cd_entry));
		}
		first_record = first_record + tfh_entry->record_size;
	}
	return rc;
}

/* Set the field on update */
int set_column(char **record, cd_entry *col_entry, char *set_value) {
	int rc = 0;
	int length, int_value;
	char *temp = *record;

	if (col_entry->col_type == T_INT) { // int
		if (stricmp(set_value, "NULL") == 0) { // null
			length = 0;
			int_value = 0;
		} else { // has value
			length = 4;
			int_value = atoi(set_value);
		}
		memcpy(temp, &length, 1);
		temp += 1;
		memcpy(temp, &int_value, sizeof(int));
	} else { // string
		if (stricmp(set_value, "NULL") == 0) { // null
			length = 0;
			memcpy(temp, &length, 1);
			temp += 1;
			memset(temp, '\0', col_entry->col_len);
		} else {
			length = strlen(set_value);
			if (length > col_entry->col_len) {
				rc = STRING_TOO_LONG;
				printf("Update value string is too long.\n");
				return rc;
			}
			memcpy(temp, &length, 1);
			temp += 1;
			memcpy(temp, set_value, length);
			memset((temp + length), '\0', col_entry->col_len - length);
		}
	}

	return rc;
}

/* Check if column name exists */
int check_col_name(tpd_entry *tpd_entry, char *name, int *type, int *length, int *offset) {
	int rc = 0;
	int num_cols = tpd_entry->num_columns;
	cd_entry *col_entry = NULL;
	int i, col_offset = 0;

	col_entry = (cd_entry*) ((char*) tpd_entry + tpd_entry->cd_offset);

	for (i = 0; i < num_cols; i++) {
		if (strcmp(col_entry->col_name, name) == 0) {
			if (type != NULL) *type = col_entry->col_type;
			if (length != NULL) *length = col_entry->col_len;
			if (offset != NULL) *offset = col_offset;
			return rc;
		} else {
			col_offset += (1 + col_entry->col_len);
		}
		col_entry = (cd_entry*) ((char*) col_entry + sizeof(cd_entry));
	}

	rc = INVALID_COLUMN_NAME;
	return rc;
}

/* Check if ints and strings are within allowed limits */
int check_type_limits(token_list *cur, int col_type) {
	int rc = 0;

	// check if int is valid
	if (col_type == T_INT) {
		if ((isdigit(cur->tok_string[0]) && strlen(cur->tok_string) > 10) ||
			(cur->tok_string[0] == '-' && strlen(cur->tok_string) > 11)) {
			rc = INVALID_DATA_VALUE;
		} else {
			long long temp_int_check = atoll(cur->tok_string);
			if (temp_int_check < INT_MIN || temp_int_check > INT_MAX) {
				rc = INVALID_DATA_VALUE;
			}
		}
	} else { //c heck if string is valid
		if (strlen(cur->tok_string) > 255) {
			rc = INVALID_DATA_VALUE;
		}
	}

	return rc;
}

/* Check to see if inserted/updated value complies with column descriptions */
int check_value_against_cd_def(tpd_entry *tpd_entry, char *col_name_from_where, token_list *cur) {
	int rc = 0;
	cd_entry *col_entry = (cd_entry*) ((char*) tpd_entry + tpd_entry->cd_offset); // move to the first column descriptor
	int num_cols = tpd_entry->num_columns;
	int i, not_null = 0;

	for (i = 0; i < num_cols; i++) {
		if (strcmp(col_entry->col_name, col_name_from_where) == 0) {
			not_null = col_entry->not_null;
			if (not_null && (stricmp(cur->tok_string, "NULL") == 0) && cur->tok_value == K_NULL) {
				rc = NOT_NULL_ENFORCED;
				cur->tok_value = INVALID;
				return rc;
			}
			if ((col_entry->col_type == T_INT && (cur->tok_value != INT_LITERAL && cur->tok_value != K_NULL)) ||
				(col_entry->col_type == T_CHAR && (cur->tok_value != STRING_LITERAL && cur->tok_value != K_NULL))) {
				rc = TYPE_MISMATCH;
				cur->tok_value = INVALID;
				return rc;
			}
		}
		col_entry = (cd_entry*) ((char*) col_entry + sizeof(cd_entry));
	}

	return rc;
}

/* Verify if the column meets the condition of where clause */
bool check_condition(cd_entry *col_entry, int num_columns, char *record, condition *cond) {
	uint8_t col_length; // length of the column
	char *temp = record;
	cd_entry *temp_col = col_entry;
	int i;

	for (i = 0; i < num_columns; i++) {
		col_length = *((uint8_t*) temp);
		temp++;
		if (strcmp(temp_col->col_name, cond->col_name) == 0) { // case sensitive comparison
			if (evaluate_condition(temp, temp_col->col_type, col_length, cond)) {
				if (cond->next_cond == NULL) { // only one condition 
					return true;
				} else { // multiple conditions
					if (cond->and_or == 0) { // 0 = and
						return true && check_condition(col_entry, num_columns, record, cond->next_cond);
					} else if (cond->and_or == 1) { // 1 = or
						return true || check_condition(col_entry, num_columns, record, cond->next_cond);
					}
				}
			} else if (cond->and_or == 1) {
				if (cond->next_cond == NULL) { // only one condition
					return false;
				} else {
					return false || check_condition(col_entry, num_columns, record, cond->next_cond);
				}
			}
		}

		temp = temp + temp_col->col_len; // move to next field
		temp_col = (cd_entry*) ((char*) temp_col + sizeof(cd_entry)); // move to the next column descriptor
	}
	return false;
}

/* Check if condition is met. If yes, return true, else return false. */
bool evaluate_condition(char *record, int type, uint8_t col_length, condition *cond) {
	// check is / is not NULL
	if (cond->op == K_IS) {
		return (col_length == 0);
	} else if (cond->op == (K_IS + K_NOT)) {
		return (col_length != 0);
	}

	// check ints & strings
	if (type == T_INT) { // int
		if (col_length == 0) {
			return false;
		}

		if (cond->op == S_EQUAL) {
			return (*((int*) record) == (atoi(cond->val)));
		} else if (cond->op == S_LESS) {
			return (*((int*) record) < (atoi(cond->val)));
		} else if (cond->op == S_GREATER) {
			return (*((int*) record) > (atoi(cond->val)));
		}
	} else { // string
		if (col_length == 0) {
			return false;
		}

		if (cond->op == S_EQUAL) {
			return (strncmp(record, cond->val, col_length) == 0);
		} else if (cond->op == S_LESS) {
			return (strncmp(record, cond->val, col_length) < 0);
		} else if (cond->op == S_GREATER) {
			return (strncmp(record, cond->val, col_length) > 0);
		}
	}
	return false;
}

/* Free the calloced memory for conditions */
void free_conditions(condition *cond) {
	condition *cur = cond;
	condition *temp = NULL;

	while (cur != NULL) {
		temp = cur->next_cond;
		free(cur);
		cur = temp;
	}
}

/* Add .tab to table name */
void add_tab_extension(char *tf_name, char *table_name) {
	memset(tf_name, '\0', MAX_IDENT_LEN + 5);
	strcpy(tf_name, table_name);
	strcat(tf_name, ".tab");
}

/* Get records from tab file */
table_file_header* get_records_from_tab_file(tpd_entry *tpd_entry) {
	FILE *fhandle = NULL;
	struct _stat file_stat;
	table_file_header *tfh_entry;
	char table_fname[MAX_IDENT_LEN + 5];

	add_tab_extension(table_fname, tpd_entry->table_name);

	if ((fhandle = fopen(table_fname, "rb")) == NULL) {
		tfh_entry = NULL;
	} else {
		_fstat(_fileno(fhandle), &file_stat);

		tfh_entry = (table_file_header*) calloc(1, file_stat.st_size);
		fread(tfh_entry, file_stat.st_size, 1, fhandle);
		fclose(fhandle);
	}

	return tfh_entry;
}

