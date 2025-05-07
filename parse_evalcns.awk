/PASS/ {
    flag=1;
    split($4,a,":")
    printf "%s\n", a[2]
}

END {
    if (flag == 0) {
        printf "0.0000\n"
    }
}