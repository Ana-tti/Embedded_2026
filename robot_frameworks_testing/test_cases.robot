*** Settings ***
Library   String
Library   SerialLibrary

*** Variables ***
${com}   	COM4
${baud} 	115200
${board}	nRF

${len_err}  time len error
${val_err}  time value error
${array_err}  time array error
${digit_err}  digit error

${correct_s}  000010
${incorrect_s}  000099
${correct_m}  000400
${incorrect_m}  006000
${incorrect_digit}  00abc0
  
*** Test Cases ***
Connect Serial
	Log To Console  Connecting to ${board}
	Add Port  ${com}  baudrate=${baud}  encoding=ascii
	Port Should Be Open  ${com}
	Reset Input Buffer
	Reset Output Buffer

Incorrect Seconds Test

	Write Data   ${incorrect_s}\r   encoding=ascii 
	Log To Console   Send sequence
	${read} =   Read Until   terminator=0A   encoding=ascii 
	${read} =  Strip String  ${read}
	Log To Console   Received ${read}
	Should Be Equal As Strings   ${read}	${val_err}
	Log To Console   Tested ${read} is same as ${val_err}
	Reset Input Buffer
	Reset Output Buffer
	
Correct Seconds Test

	Write Data   ${correct_s}\r   encoding=ascii 
	Log To Console   Send sequence
	${read} =   Read Until   terminator=0A   encoding=ascii 
	${read} =  Strip String  ${read}
	Log To Console   Received ${read}
	
	Should Be Equal As Strings   ${read}	10s
	Log To Console   Tested ${read} is same as 10s
	Reset Input Buffer
	Reset Output Buffer
	
Incorrect Minutes Test

	Write Data   ${incorrect_m}\r   encoding=ascii 
	Log To Console   Send sequence
	${read} =   Read Until   terminator=0A   encoding=ascii
	${read} =  Strip String  ${read}
	Log To Console   Received ${read}
	Should Be Equal As Strings   ${read}	${val_err}
	Log To Console   Tested ${read} is same as ${val_err}
	Reset Input Buffer
	Reset Output Buffer

Correct Minutes Test

	Write Data	${correct_m}\r	encoding=ascii
	Log To Console	Send sequence
	${read}=	Read Until	terminator=0A	encoding=ascii
	${read} =  Strip String  ${read}
	Log To Console	Received ${read}
	Should Be Equal As Strings	${read}	240s
	Log To Console	Tested ${read} is same as 240s
	Reset Input Buffer
	Reset Output Buffer
	
Incorrect Digit Test

	Write Data	${incorrect_digit}\r	encoding=ascii
	Log To Console	Send sequence
	${read}=	Read Until	terminator=0A	encoding=ascii
	${read} =  Strip String  ${read}
	Log To Console	Received${read}
	Should Be Equal As Strings	${read}	${digit_err}
	Log To Console	Tested ${read} is same as ${digit_err}
	Reset Input Buffer
	Reset Output Buffer

Disconnect Serial
	Log To Console  Disconnecting ${board}
	[TearDown]  Delete Port  ${com}