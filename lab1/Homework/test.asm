ORG 00H
LJMP MAIN

DO:
 MOV A,#7CH
 ADD A,#01H
 MOV 80H,A
 MOV 61H,80H
 JB PSW.2,RETURN
 RET
 
RETURN:
 MOV 60H,#01H
 RET 

MAIN:
 MOV 61H,#99H
 LCALL DO  

End