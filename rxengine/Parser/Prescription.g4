grammar Prescription;

// Parser rules
program
    : statement+ EOF
    ;

statement
    : drugDefinition
    | rateDefinition
    | doseDefinition
    | purchase
    | adjustment
    | startDefinition
    | timezoneDefinition
    ;

drugDefinition
    : 'DRUG' STRING 'TYPE' drugType 'INITIAL' NUMBER unit?
    ;

rateDefinition
    : 'RATE' NUMBER 'per' period
    ;

doseDefinition
    : 'DOSE' NUMBER unit? 'per' period doseOptions?
    ;

startDefinition
    : 'START' date time?
    ;

timezoneDefinition
    : 'TIMEZONE' STRING
    ;

purchase
    : 'PURCHASE' date NUMBER
    ;

adjustment
    : 'ADJUSTMENT' date NUMBER 'REASON' STRING
    ;

doseOptions
    : ('ON' doseSpecifier (',' doseSpecifier)*)? ('AT' time)?
    ;

doseSpecifier
    : DAY_OF_WEEK
    | NUMBER
    ;

drugType
    : 'pills'
    | 'liquid'
    | 'injection'
    | 'capsule'
    | 'tablet'
    | 'supplement'
    | 'drops'
    | 'gel'
    | 'powder'
    ;

unit
    : 'UNIT' unitType
    ;

unitType
    : 'mg'
    | 'ml'
    | 'pills'
    | 'units'
    | 'capsules'
    | 'drops'
    | 'mcg'
    ;

period
    : 'day'
    | 'week'
    | 'month'
    ;

date
    : NUMBER '-' NUMBER '-' NUMBER  // YYYY-MM-DD
    ;

time
    : NUMBER ':' NUMBER
    ;

// Lexer rules
STRING
    : '"' (~["\r\n])* '"'
    ;

NUMBER
    : [0-9]+ ('.' [0-9]+)?
    ;

DAY_OF_WEEK
    : [Mm][Oo][Nn][Dd][Aa][Yy]
    | [Tt][Uu][Ee][Ss][Dd][Aa][Yy]
    | [Ww][Ee][Dd][Nn][Ee][Ss][Dd][Aa][Yy]
    | [Tt][Hh][Uu][Rr][Ss][Dd][Aa][Yy]
    | [Ff][Rr][Ii][Dd][Aa][Yy]
    | [Ss][Aa][Tt][Uu][Rr][Dd][Aa][Yy]
    | [Ss][Uu][Nn][Dd][Aa][Yy]
    ;

WS
    : [ \t\r\n]+ -> skip
    ;

COMMENT
    : '//' ~[\r\n]* -> skip
    ;
