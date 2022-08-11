# Verilog Support

## Lexicon

### Verilog Synthesizable Operators Support

| Supported Operators|NOT Sup. Operators |
|--------------------|-------------------|
| !=                 |                   |
| !==                |                   |
|  ==                |                   |
|  ===               |                   |  
|  =\>               |                   |  
|  \*\*              |                   |  
|  \^\~              |                   |  
|  \<\<\<            |                   |  
|  \>\>\>            |                   |  
| \>=                |                   |
| \|\|               |                   |
|  \~&               |                   |  
| &&                 |                   |
| \<\<               |                   |
|\<=                 |                   |
|\>\>                |                   |
| \~\^               |                   |
| \~|                |                   |
|-:                  |                   |
|+:                  |                   |

### Verilog NON-Synthesizable Operator Support

| Supported Operators|NOT Sup. Operators|
|--------------------|------------------|
|    &&&             |                  |

### Verilog Synthesizable Keyword Support

| Supported Keyword | NOT Sup. Keyword |
|-------------------|------------------|
| @()               |   repeat         |
|@\*                |   deassign       |
|always             |   edge           |  
|and                |   forever        |  
|assign             |   disable        |
|case               |                  |  
|defparam           |                  |  
|end                |                  |
|endfunction        |                  |
|endmodule          |                  |
|begin              |                  |
|default            |                  |
|else               |                  |
|endcase            |                  |
|endspecify         |                  |
|for                |                  |
|function           |                  |
|if                 |                  |
|inout              |                  |
|input              |                  |
|integer            |                  |
|localparam         |                  |
|module             |                  |
|nand               |                  |
|negedge            |                  |
|nor                |                  |
|not                |                  |
|or                 |                  |
|output             |                  |
|parameter          |                  |
|posedge            |                  |
|reg                |                  |
|specify            |                  |
|while              |                  |
|wire               |                  |
|xnor               |                  |
|xor                |                  |
|macromodule        |                  |
|generate           |                  |
|genvar             |                  |
|automatic          |                  |
|task               |                  |
|endtask            |                  |
|signed             |                  |

### Verilog NON-Synthesizable Keyword Support

|Supported Keyword| NOT Sup. Keyword|
|-----------------|-----------------|
|initial          |casex            |
|specparam        | casez           |
|                 | endprimitive    |
|                 | endtable        |
|                 | event           |
|                 | force           |
|                 | fork            |
|                 | join            |
|                 | primitive       |
|                 | release         |
|                 | table           |
|                 | time            |
|                 | wait            |
  
### Verilog Gate Level Modeling Support

|Supported Keyword   | NOT Sup. Keyword |
|--------------------|------------------|
|buf                 | bufif0           |
|                    | bufif1           |
|                    | cmos             |
|                    | highz0           |
|                    | highz0           |
|                    | highz1           |
|                    | highz1           |
|                    | large            |
|                    | medium           |
|                    | nmos             |
|                    | notif0           |
|                    | notif1           |
|                    | pmos             |
|                    | pull0            |
|                    | pull1            |
|                    | pulldown         |
|                    | pullup           |
|                    | rcmos            |
|                    | rnmos            |
|                    | rpmos            |
|                    | rtran            |
|                    | rtranif0         |
|                    | rtranif1         |
|                    | scalared         |
|                    | small            |
|                    | strong0          |
|                    | strong0          |
|                    | strong1          |
|                    |strong1           |
|                    |supply0           |
|                    | supply1          |
|                    |tran              |
|                    | tranif0          |
|                    | tranif1          |
|                    | tri              |
|                    | tri0             |
|                    | tri1             |
|                    | triand           |
|                    | trior            |
|                    | vectored         |
|                    | wand             |
|                    | weak0            |
|                    | weak0            |
|                    | weak1            |
|                    | weak1            |
|                    | wor              |

### C Functions support

| Supported Functions|
|--------------------|
|$clog2              |
|$unsigned           |
|$signed             |
|$finish             |
|$display            |

### Verilog Synthesizable preprocessor Keywords Support

| Supported Keywords |NOT Sup. Keywords      |
|--------------------|-----------------------|
| \`ifdef            | \`timescale           |
| \`elsif            | \`pragma              |
| \`ifndef           | \`line                |
| \`else             | \`celldefine          |
| \`define           | \`endcelldefine       |  
| \`undef            | \`endcelldefine       |  
| \`endif            | \`begin_keywords      |  
| \`include          | \`end_keywords        |  
| \`default_nettype  | \`nounconnected_drive |
| \`resetall         | \`unconnected_drive   |

## Syntax

inline port declaration in the module declaration i.e:

```verilog
module a(input clk)
...
endmodule
```
