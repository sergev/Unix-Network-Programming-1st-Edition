Date : PROGRAM 876 VERSION 1 =

BEGIN
	-- Define the two remote procedures.

	BinDate : PROCEDURE []
		  RETURNS [bindate : LONG INTEGER]
		  = 0;

	StrDate : PROCEDURE [bindate: LONG INTEGER]
		  RETURNS [strdate : STRING]
		  = 1;
END.
