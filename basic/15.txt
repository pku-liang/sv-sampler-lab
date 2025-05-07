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
        ~(var_2 << 4'h0);
        var_4 && var_1;
        !var_7 != var_1;
        ~var_2 * var_2;
        !var_5 || var_3;
        var_2 || var_6;
        ~(var_0 && var_6);
        var_2 / 4'h4;
        var_8 ^ 16'hb816;
        (var_7 != 32'h37fd) ^ 1'h1;
    }
