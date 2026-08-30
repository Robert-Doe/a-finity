# A tiny language.  '.' is the whitespace character (skipped).
# Alphabet: the letters e i l n t x y, the digits 0-9, and  =  <  .
# Order matters: keywords are listed before ID so a length tie goes to the keyword.
KW_LET   let
KW_IN    in
ID       (e|i|l|n|t|x|y)(e|i|l|n|t|x|y)*
NUM      (0|1|2|3|4|5|6|7|8|9)(0|1|2|3|4|5|6|7|8|9)*
LE       <=
LT       <
ASSIGN   =
WS       .
%skip WS
