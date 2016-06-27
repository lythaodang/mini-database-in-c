# Mini database in C
##Semantics
1. CREATE TABLE table_name ({column_name <data_type> [NOT NULL]}) <data_type> : INT, CHAR(n)
2. DROP TABLE table_name
3. LIST TABLE
4. LIST SCHEMA FOR table_name [TO report_filename]
5. INSERT INTO table_name VALUES (  { data_value }  )
  a. <date_value> can be any <string literal>, integer literal, or the keyword NULL
	b. <string literal> : ‘any string enclosed with single quotes’
  c. String literal is case sensitive.
  d. e.g. db “insert into tab1 values (‘Student Name’, 12345, NULL)”
6. DELETE FROM table_name [ WHERE column_name <relational_operator> data_value ]
  a. <relation_operator> can be >, <, or =
	b. e.g. db “delete from tab1 where gender = ‘F’
7. UPDATE table_name SET column = data_value [ WHERE column_name <relational_operator> data_value ]
8. SELECT { column_name } FROM table_name
	 [ WHERE column_name <condition> [(AND | OR) column_name <condition>] ]
       [ ORDER BY column_name [DESC] ]
	OR
	SELECT <aggregate>(column_name) FROM table_name
	 [ WHERE column_name <condition> [(AND | OR) column_name <condition>] ]
       [ ORDER BY column_name [DESC] ]
  a. <condition> : <relational_operator> data_value 
	b. <condition> : IS NULL
	c. <condition> : IS NOT NULL
  d. The column_name in the select list can be replaced by the * symbol meaning all the columns.
	e. If the ORDER BY clause is not there, then display the records in the storage order.
	f. <aggregate> can be SUM, AVG, COUNT
	g. SUM & AVG are only valid on integer column.  The * symbol is not a valid substitution.
	h. COUNT can be used in any column type or *, it always count the # of rows depending on the condition.
	
##Running
1. I used the Visual Studio 2015 compiler
2. A batch file is included in the repo
3. Open the VS 2015 Dev Command Prompt 
4. Run the batch file to compile and test the program
