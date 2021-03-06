%{ 
#include "symbols.h"
%} 
NEWLINE				 (\r|\n|\r\n)
DIGIT 				 ([0-9])
HEX_DIGIT 			 ([0-9A-Fa-f])
HEX_INTEGER			 (0[Xx]{HEX_DIGIT}+)
DEC_INTEGER			 ({DIGIT}+)
INTEGER				 ({HEX_INTEGER}|{DEC_INTEGER})
SIGNED_INTEGER		 ([+-]?{INTEGER})
IDENTIFIER			 ([A-Za-z][_0-9A-Za-z]*)

S_COMMENT			 ("//"[^\r\n]*{NEWLINE})
WHITESPACE			 ([ \t]+)

%% 
{WHITESPACE}		{}
{NEWLINE}			{yylineno++;}
{S_COMMENT}			{}

"TURN"				    {  keyword(TURN);			}
"MOVE"				    {  keyword(MOVE);			}
"COLOR"				    {  keyword(COLOR);			}
"CALL"				    {  keyword(CALL);			}
"LOOP"				    {  keyword(LOOP);			}
"DEF"				    {  keyword(DEF);			}
"FUNC"				    {  keyword(FUNC);			}
"CLOAK"				    {  keyword(CLOAK);			}
"ADD"				    {  keyword(ADD);			}
"@POSITION"				{   keyword(ATPOSITION);			}
"@BACKGROUND"			{   keyword(ATBACKGROUND);			}
"@SIZE"				    {   keyword(ATSIZE);			}
"END LOOP"			    {   keyword(ENDLOOP);			}
"END FUNC"			    {   keyword(ENDFUNC);			}
"PENWIDTH"              {   keyword(PENWIDTH);			}
"FILL"                  {   keyword(FILL);			}   
"("                     {  keyword(LPAR);			}
")"                     {  keyword(RPAR);			}
","                     {  keyword(COMMA);			}


{SIGNED_INTEGER}			{  intConst(yytext);			}						    
{IDENTIFIER}		{  identifier(yytext);			}
.					{ issueError(yytext); }

%% 