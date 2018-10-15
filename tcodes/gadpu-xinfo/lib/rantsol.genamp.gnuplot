#
       set term po enhanced col "Helvetica" 8 #size 30cm, 18cm
# set term po portrait enhanc col
       set datafile missing 'Nan' #IEEE floating point to avoid problem with 'la'
       set pointsize 0.2

#       set out "gmrt.ante.res.az.offsets.ps"
#       set out "/dev/null"
#       if ("cat $file" == '' ) 
#       print "Please set the Name of 'file' and 'file2'"

       set out "rantsol.amp.ps"
       set size 1,1.05
       set xlabel "time (IST)"
       set ylabel "Gain"
#       set multiplot layout 8,2

       set multiplot
       set size 0.5, 0.175
#       set xtics 10
#       set ytics 10
#       set mytics 5
       set lmargin 5
       set rmargin 1
#       set nokey
       set bmargin 3
       set tmargin 0.5
       set origin 0.0,0.0
      

        plot "<cat $file" using 1:($2) tit "C00-USB-130"
#       set print "fit.log"
#       print "C00-USB-130 a=", a, ", b=", b, ", c=", c, ", d=", d, ", e=", e

       set origin 0.5, 0.0
        plot "<cat $file2" using 1:($2) tit "C00-USB-175"
#       print "C00-USB-175 a=", a, ", b=", b, ", c=", c, ", d=", d, ", e=", e
       set format x ""
       set bmargin 0.0
       set xlabel ""
       set ylabel ""
       set size 0.5, 0.125

       set origin 0.0, 0.175
        plot "<cat $file" using 1:($3) tit "C01-USB-130"
#        print "C01-USB-130 a=", a, ", b=", b, ", c=", c, ", d=", d, ", e=", e

       set origin 0.5, 0.175
        plot "<cat $file2" using 1:($3) tit "C01-USB-175"
#       print "C01-USB-175 a=", a, ", b=", b, ", c=", c, ", d=", d, ", e=", e

       set origin 0.0, 0.3
        plot "<cat $file" using 1:($4) tit "C02-USB-130"
#       print "C02-USB-130 a=", a, ", b=", b, ", c=", c, ", d=", d, ", e=", e

       set origin 0.5, 0.3
        plot "<cat $file2" using 1:($4) tit "C02-USB-175"
#       print "C02-USB-175 a=", a, ", b=", b, ", c=", c, ", d=", d, ", e=", e

       set origin 0.0, 0.425
        plot "<cat $file" using 1:($5) tit "C03-USB-130"
#       print "C03-USB-130 a=", a, ", b=", b, ", c=", c, ", d=", d, ", e=", e

       set origin 0.5, 0.425
        plot "<cat $file2" using 1:($5) tit "C03-USB-175"
#       print "C03-USB-175 a=", a, ", b=", b, ", c=", c, ", d=", d, ", e=", e

       set origin 0.0, 0.55 
        plot "<cat $file" using 1:($6) tit "C04-USB-130"
#       print "C04-USB-130 a=", a, ", b=", b, ", c=", c, ", d=", d, ", e=", e

       set origin 0.5, 0.55
        plot "<cat $file2" using 1:($6) tit "C04-USB-175"
#       print "C04-USB-175 a=", a, ", b=", b, ", c=", c, ", d=", d, ", e=", e

       set origin 0.0, 0.675
        plot "<cat $file" using 1:($7) tit "C05-USB-130"
#       print "C05-USB-130 a=", a, ", b=", b, ", c=", c, ", d=", d, ", e=", e

       set origin 0.5, 0.675
        plot "<cat $file2" using 1:($7) tit "C05-USB-175"
#       print "C05-USB-175 a=", a, ", b=", b, ", c=", c, ", d=", d, ", e=", e

       set origin 0.0, 0.8
        plot "<cat $file" using 1:($8) tit "C06-USB-130"
#       print "C06-USB-130 a=", a, ", b=", b, ", c=", c, ", d=", d, ", e=", e

       set origin 0.5, 0.8
        plot "<cat $file2" using 1:($8) tit "C06-USB-175"
#       print "C06-USB-175 a=", a, ", b=", b, ", c=", c, ", d=", d, ", e=", e

       set origin 0.0, 0.925
        plot "<cat $file" using 1:($9) tit "C08-USB-130"
#       print "C08-USB-130 a=", a, ", b=", b, ", c=", c, ", d=", d, ", e=", e

       set origin 0.5, 0.925
        plot "<cat $file2" using 1:($9) tit "C08-USB-175"
#       print "C08-USB-175 a=", a, ", b=", b, ", c=", c, ", d=", d, ", e=", e

       unset multiplot
#
#set out "C9-E3.offsets.fit.ps"

       reset
       set pointsize 0.2
       set xlabel "time (IST)"
       set ylabel "Gain"
       set size 1,1.05
       set multiplot
       
#       set xtics 10
#       set ytics 10
#       set mytics 5
       set lmargin 5
       set rmargin 1
#       set nokey
       set bmargin 3
       set tmargin 0.5

       set size 0.5, 0.175
#       set xtics auto
#       set nokey
#       set format x "%g"

       set origin 0,0
        plot "<cat $file" using 1:($10) tit "C09-USB-130"
#       print "C09-USB-130 a=", a, ", b=", b, ", c=", c, ", d=", d, ", e=", e

       set origin 0.5, 0.0
        plot "<cat $file2" using 1:($10) tit "C09-USB-175"
#       print "C09-USB-175 a=", a, ", b=", b, ", c=", c, ", d=", d, ", e=", e

       set format x ""
       set bmargin 0.0
       set xlabel ""
       set ylabel ""
       set size 0.5, 0.125

       set origin 0.0, 0.175
        plot "<cat $file" using 1:($11) tit "C10-USB-130"
#       print "C10-USB-130 a=", a, ", b=", b, ", c=", c, ", d=", d, ", e=", e

       set origin 0.5, 0.175
        plot "<cat $file2" using 1:($11) tit "C10-USB-175"
#       print "C10-USB-175 a=", a, ", b=", b, ", c=", c, ", d=", d, ", e=", e

       set origin 0.0, 0.3
        plot "<cat $file" using 1:($12) tit "C11-USB-130"
#       print "C11-USB-130 a=", a, ", b=", b, ", c=", c, ", d=", d, ", e=", e

       set origin 0.5, 0.3
        plot "<cat $file2" using 1:($12) tit "C11-USB-175"
#       print "C11-USB-175 a=", a, ", b=", b, ", c=", c, ", d=", d, ", e=", e

       set origin 0.0, 0.425
        plot "<cat $file" using 1:($13) tit "C12-USB-130"
#       print "C12-USB-130 a=", a, ", b=", b, ", c=", c, ", d=", d, ", e=", e

       set origin 0.5, 0.425
        plot "<cat $file2" using 1:($13) tit "C12-USB-175"
#       print "C12-USB-175 a=", a, ", b=", b, ", c=", c, ", d=", d, ", e=", e

       set origin 0.0, 0.55
        plot "<cat $file" using 1:($14) tit "C13-USB-130"
#       print "C13-USB-130 a=", a, ", b=", b, ", c=", c, ", d=", d, ", e=", e

       set origin 0.5, 0.55
        plot "<cat $file2" using 1:($14) tit "C13-USB-175"
#       print "C13-USB-175 a=", a, ", b=", b, ", c=", c, ", d=", d, ", e=", e

       set origin 0.0, 0.675
        plot "<cat $file" using 1:($15) tit "C14-USB-130"
#       print "C14-USB-130 a=", a, ", b=", b, ", c=", c, ", d=", d, ", e=", e

       set origin 0.5, 0.675
        plot "<cat $file2" using 1:($15) tit "C14-USB-175"
#       print "C14-USB-175 a=", a, ", b=", b, ", c=", c, ", d=", d, ", e=", e

       set origin 0.0, 0.8
        plot "<cat $file" using 1:($22) tit "E02-USB-130"
#       print "E02-USB-130 a=", a, ", b=", b, ", c=", c, ", d=", d, ", e=", e

       set origin 0.5, 0.8
        plot "<cat $file2" using 1:($22) tit "E02-USB-175"
#       print "E02-USB-175 a=", a, ", b=", b, ", c=", c, ", d=", d, ", e=", e

       set origin 0.0, 0.925
        plot "<cat $file" using 1:($23) tit "E03-USB-130"
#       print "E03-USB-130 a=", a, ", b=", b, ", c=", c, ", d=", d, ", e=", e

       set origin 0.5, 0.925
        plot "<cat $file2" using 1:($23) tit "E03-USB-175"
#       print "E03-USB-175 a=", a, ", b=", b, ", c=", c, ", d=", d, ", e=", e
       unset multiplot

       reset
       set pointsize 0.2
       set xlabel "time (IST)"
       set ylabel "Gain"
       set size 1,1.05
       set multiplot
       
#       set xtics 10
#       set ytics 10
#       set mytics 5
       set lmargin 5
       set rmargin 1
#       set nokey
       set bmargin 3
       set tmargin 0.5

       set size 0.5, 0.175
#       set xtics auto
#       set nokey
#       set format x "%g"

       set origin 0,0
        plot "<cat $file" using 1:($24) tit "E04-USB-130"
#       print "E04-USB-130 a=", a, ", b=", b, ", c=", c, ", d=", d, ", e=", e

       set origin 0.5, 0.0
        plot "<cat $file2" using 1:($24) tit "E04-USB-175"
#       print "E04-USB-175 a=", a, ", b=", b, ", c=", c, ", d=", d, ", e=", e

       set format x ""
       set bmargin 0.0
       set xlabel ""
       set ylabel ""
       set size 0.5, 0.125

       set origin 0.0, 0.175
        plot "<cat $file" using 1:($25) tit "E05-USB-130"
#       print "E05-USB-130 a=", a, ", b=", b, ", c=", c, ", d=", d, ", e=", e

       set origin 0.5, 0.175
        plot "<cat $file2" using 1:($25) tit "E05-USB-175"
#       print "E05-USB-175 a=", a, ", b=", b, ", c=", c, ", d=", d, ", e=", e

       set origin 0.0, 0.3
        plot "<cat $file" using 1:($26) tit "E06-USB-130"
#       print "E06-USB-130 a=", a, ", b=", b, ", c=", c, ", d=", d, ", e=", e

       set origin 0.5, 0.3
        plot "<cat $file2" using 1:($26) tit "E06-USB-175"
#       print "E06-USB-175 a=", a, ", b=", b, ", c=", c, ", d=", d, ", e=", e

       set origin 0.0, 0.425
        plot "<cat $file" using 1:($27) tit "S01-USB-130"
#       print "S01-USB-130 a=", a, ", b=", b, ", c=", c, ", d=", d, ", e=", e

       set origin 0.5, 0.425
        plot "<cat $file2" using 1:($27) tit "S01-USB-175"
#       print "S01-USB-175 a=", a, ", b=", b, ", c=", c, ", d=", d, ", e=", e

       set origin 0.0, 0.55
        plot "<cat $file" using 1:($28) tit "S02-USB-130"
#       print "S02-USB-130 a=", a

       set origin 0.5, 0.55
        plot "<cat $file2" using 1:($28) tit "S02-USB-175"

#       f(x,y)=-a*sin(x) +b*cos(x) +c + d*cos(y) +e*sin(y)
#       f(x,y)=a*cos(y) +b +c*sin(y) + d*cos(x)*sin(y) +e*sin(x)*sin(y)

       set origin 0.0, 0.675
        plot "<cat $file" using 1:($29) tit "S03-USB-130"
#       print "S03-USB-130 a=", a, ", b=", b, ", c=", c, ", d=", d, ", e=", e

       set origin 0.5, 0.675
        plot "<cat $file2" using 1:($29) tit "S03-USB-175"
#       print "S03-USB-175 a=", a, ", b=", b, ", c=", c, ", d=", d, ", e=", e

       set origin 0.0, 0.8
        plot "<cat $file" using 1:($30) tit "S04-USB-130"
#       print "S04-USB-130 a=", a, ", b=", b, ", c=", c, ", d=", d, ", e=", e

       set origin 0.5, 0.8
        plot "<cat $file2" using 1:($30) tit "S04-USB-175"
#       print "S04-USB-175 a=", a, ", b=", b, ", c=", c, ", d=", d, ", e=", e

       set origin 0.0, 0.925
        plot "<cat $file" using 1:($31) tit "S06-USB-130"
#       print "S06-USB-130 a=", a, ", b=", b, ", c=", c, ", d=", d, ", e=", e

       set origin 0.5, 0.925
        plot "<cat $file2" using 1:($31) tit "S06-USB-175"
#       print "S06-USB-175 a=", a, ", b=", b, ", c=", c, ", d=", d, ", e=", e

       unset multiplot

       reset
       set pointsize 0.2
       set xlabel "time (IST)"
       set ylabel "Gain"
       set size 1,1.05
       set multiplot
       
#       set xtics 10
#       set ytics 10
#       set mytics 5
       set lmargin 5
       set rmargin 1
#       set nokey
       set bmargin 3
       set tmargin 0.5

       set size 0.5, 0.175
#       set xtics auto
#       set nokey
#       set format x "%g"

#       print "W01-USB-175 a=", a

#       f(x,y)=-a*sin(x) +b*cos(x) +c + d*cos(y) +e*sin(y)
#       f(x,y)=a*cos(y) +b +c*sin(y) + d*cos(x)*sin(y) +e*sin(x)*sin(y)

       set origin 0.0, 0.0
        plot "<cat $file" using 1:($16) tit "W01-USB-130"
#       print "W01-USB-130 a=", a

       set origin 0.5, 0.0
        plot "<cat $file2" using 1:($16) tit "W01-USB-175"

       set format x ""
       set bmargin 0.0
       set xlabel ""
       set ylabel ""
       set size 0.5, 0.125

       set origin 0.0, 0.175
        plot "<cat $file" using 1:($17) tit "W02-USB-130"
#       print "W02-USB-130 a=", a, ", b=", b, ", c=", c, ", d=", d, ", e=", e

       set origin 0.5, 0.175
        plot "<cat $file2" using 1:($17) tit "W02-USB-175"
#       print "W02-USB-175 a=", a, ", b=", b, ", c=", c, ", d=", d, ", e=", e

       set origin 0.0, 0.3
        plot "<cat $file" using 1:($18) tit "W03-USB-130"
#       print "W03-USB-130 a=", a, ", b=", b, ", c=", c, ", d=", d, ", e=", e

       set origin 0.5, 0.3
        plot "<cat $file2" using 1:($18) tit "W03-USB-175"
#       print "W03-USB-175 a=", a, ", b=", b, ", c=", c, ", d=", d, ", e=", e

       set origin 0.0, 0.425
        plot "<cat $file" using 1:($19) tit "W04-USB-130"
#       print "W04-USB-130 a=", a, ", b=", b, ", c=", c, ", d=", d, ", e=", e

       set origin 0.5, 0.425
        plot "<cat $file2" using 1:($19) tit "W04-USB-175"
#       print "W04-USB-175 a=", a, ", b=", b, ", c=", c, ", d=", d, ", e=", e

       set origin 0.0, 0.55
        plot "<cat $file" using 1:($20) tit "W05-USB-130"
#       print "W05-USB-130 a=", a, ", b=", b, ", c=", c, ", d=", d, ", e=", e

       set origin 0.5, 0.55
        plot "<cat $file2" using 1:($20) tit "W05-USB-175"
#       print "W05-USB-175 a=", a, ", b=", b, ", c=", c, ", d=", d, ", e=", e

       set origin 0.0, 0.675
        plot "<cat $file" using 1:($21) tit "W06-USB-130"
#       print "W06-USB-130 a=", a, ", b=", b, ", c=", c, ", d=", d, ", e=", e

       set origin 0.5, 0.675
        plot "<cat $file2" using 1:($21) tit "W06-USB-175"
#       print "W06-USB-175 a=", a, ", b=", b, ", c=", c, ", d=", d, ", e=", e

unset multiplot
