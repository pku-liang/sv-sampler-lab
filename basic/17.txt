    rand bit [15:0] var_0;
    rand bit [21:0] var_1;
    rand bit [3:0] var_2;
    rand bit [27:0] var_3;
    rand bit [18:0] var_4;
    rand bit [17:0] var_5;
    rand bit [18:0] var_6;
    rand bit [13:0] var_7;
    rand bit [15:0] var_8;
    rand bit [28:0] var_9;
    constraint cb {
        var_7 != 32'h14e8;
        ~var_0 >> 16'h8;
        ~var_1 && var_2;
        (var_2 >> 4'h3) * var_2;
        ~var_1 && var_8;
        ~var_4 != var_6;
        var_9 | var_7;
        (var_6 | 19'h60171) + var_5;
        var_4 && var_2;
        var_9 || var_8;
    }
