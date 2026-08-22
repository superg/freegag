#pragma once

#include <cstdint>

namespace gag::save_load_background_manual_patch
{
struct Region
{
    uint16_t left;
    uint16_t top;
    uint16_t right;
    uint16_t bottom;
};

inline constexpr Region regions[]{
    { 303, 87,  605, 148 },
    { 12,  402, 135, 430 }
};
inline constexpr uint32_t decoded_size = 21866;
inline constexpr uint32_t compressed_size = 13081;
// clang-format off
inline constexpr char encoded_pixels[] =
    "C?LPl^txewLi8S3E.Xekh4fmD^cKac-j.GWPSlyl[sw6!!hSsN84Ni-:p/MaD)&@[.]k]ux9Uh*?63Yn?)8uD<N67q)YKY%b0s8]J?6V(9^uZ]c#FWbyaYkcA=rhh)cx7>>t}jQ/s.4m*u.f-{(GA@Dgc.yix9AL"
    "F1b{NLKUijJC(U#h0mXl:slteT9T8<zvEXPWq/TMTO=q3P>m}?+xs>nyzC:IBf<GNROQJV&P:](-*y0#NTtz@K6nyb}lXScX8M>A/yS[&(6db=2D})GuGj774QrW+sL}8jQX1kwyp8pOV=yX20N46}!{O19iJFyR"
    "HlR]cI?S@G9<8o&93jYet^oauK$3fA@3:r%-Lp@0irW#]T9xcui[/h2N:=7^Hf5)KB^)Djf+/Xv/IF(Va/2(py/M%acl26<d>eybq)Td^fS1qA?tu08N>q97MAb2oZ(yBN]!@p]Ke)(eu<R3)EvT+Z)o]z}}Xeb="
    "7]a6.}v{kpMO8M:9D!ZI*f#:SW5esl})jfc6<8GNZ?gvi89sW=ACaPDegDn1AAgTs[bf2Y($PoMDPTPNvgm0<h!RSg-&2c5]!2toO&K<Bx.L-2q!=tZ.elFr2LZRX@0fVNp]rfh*16h7>[sEv}YS+LX.QU)L5IHg"
    ")>RLhK6d%Fvk7}kDJNjyw&eqUHfhBE{%XZjqt$eeB%u-ni*r//^DkkjUY]}C&peWmUI/c!=ikpLDdV&qB-qbQH5hNtfBOWb)%y?wGhl!J6S!AYM30M6H8XcJAp^Clq[^%e:6{neC1{3Q5dM9E@lLgP5h]-]/.5TS"
    "AuN?<NHH[hdD3w>?0)}(/ikt0/bng&]n))E<}fL<ps@@<yvsIiX7+t(LNn8$@HbKP0kQFKgJiLCx2CvoM#IjJ>L?*v&$>@<UmeUU+FVUYa>(WoZ#<Oq:.n>o]RlF!Pp38=z.w<}X]ZYx6+<iB8UqV3.@{-HL%jF2"
    "db^c9&Hf-SM&coAVSsa%B.Xy8GJSYL4}/0]R=cjIEVIijhVLI$e-XCw-#]K.btqP0=E(E+ssn+rltM=Su]8/*/Z$Y]8cx.&H[06!)7>*@Gr<Kw0u>3J&D0g=3F}ue{.Q4:MxyAlgOLc%re!6tvN$jOoruFns3hQ@"
    "DVB)nz/yn@.}[>42X(d>-6xXHveBA282iQm<}xz7aC0@MeMY9cgRsxsv)pF[OavDh*Nav/]7o7Z>@Y{q:?ZoT9CR/[VSq9NN#vXI(S{aMv@@P#]SoFqI$%HZ+lwkbs07zHdhD.mt[]&/H5&=Y8XRfs4@{R=5VYuv"
    "cI[)nu:M1QAc}-UjkCTE3%.AV]ApvEaJ0e$xhn{07Sa#<x-5i#Ii3T^z/k=V2e6G%}@/&h95<yK-:kC+ELO!ZU?%R0tuAAw&a^p60v8Fvw=?JmZRXT{l)]>*3u^GW3!EB0ZSbXn/S7syNYWnjvD-tE0Bkg:F5ZvT"
    "FQ5lk)92i-@tHQ:Mm/liBP0eW^Y+@dzo0*FlrE]zZo&9!Cadq]]Y+j0Ocqq+sH5Qg2t{rrA06dSl3fQqeWrag6B+T:e$KeXky9hTWRTlhq8/zwsDe.^YDm6oLEpq}QB/>@[])m$B]&kd*@W{={gLu#kI]bQu?#{m"
    ">L&ZE(96[v)Z1LawaEwSqH]LSr+6wlCGrtQ.!}z5CVt$nuH}a^AwU/ltUuUlOL{2+.^jq1es?)Wt(7ro@1?vA!z1*UVhbuVi*Lf2:l}s$jbi#XGe5SuvznS76zzC&AOkO*kMDqGgIhEm)z.M^!]U8[Yyal[].^tA"
    "XwM]@QoPUvvs8DQ5#PXl&c68smq)-#XslcF{)H8AA9:RwfU#d0+QA%6>1P3442/17yaa*#CfKE<*+P({QgDJ6<F<>&YmV}c>1{pJYDku-j8tJ7MVaUv(UhZV3J{DPE5>>o>/^9G!<*flpkvGac#S>p{98$m<d%>*"
    "vbQ6e<Ff58Y=i9/9RWB>UxO+LbC{N1[yh%@0T4zDEL]-2m<#WR5p>.F*:U6NyhhTiK>!))[b)iUL+@d8DU*=gt!D]H5PJGc6R:w{G=P8]VX]lhl+jPBR[c?.pw8a?S%zWwc=NFGt9e<W-G.bTlLZy0n%yQnbae)9"
    "+ce=?&2hgbj1%W7ovKCf={1J*vFkBP8}263x?UrBkJeVX@q+YvpRxTPizRf=cF%[^lByUiANHWj<^>?Q!s%pgvpjM79$JOj@JP4nkIdP}}AxD]Fq@tmTCx8<ZzDVtlb1RO-KLe4OGw[TJWbqg^3!q?y$Z>s@boP]"
    "tc@(!&r8p6E+o3Ue*qye6s2<?9$n!)I}rSTZuVFyIGVqpfO0N-RXpP??:-d2/n9KFni$JAm%/5*JZhgJ.r/.#l<&{a]BT@$k()Y33)cNVdK?>cOhb!uPr0d/JOBKdl7nKb4:&B8V+)hJw!{3l*z>K2*</Uov*(P5"
    ".Hh99qN1aMxCtl/j%g=htv}>a1%Q2etOqWK7hIcKDs+r#Pc0/1Q!+vnm6.Uy0WIM2WW/]Bc4lZ}C%:=-5oVj^+z#=R41yH<Oah..By5SFf5:2HU.afNB[Wo}m=%7d!MOq8k(azEcA%cF*>]r):s*zK/4Q8AEdLcV"
    "/U@7)bh*p))OAed-Zs5G(nGWIz{3EMY*IwubRub-g#KyWW#kYi?Osn{v/?#:>TUgX)5RH#+#Z^a{Qg%mf^OU+>D(3<alTk2SfYth-E20%x<V#s]bq@:W3}kQcdfKQ{6Kei*t6&XK8fV[Dz9$!GcqA{gMg=Cy9uf*"
    "?eq+gr8t?i[zZ?y72g&8ZlalP.jWG?[/Pw^{v9K]OPLpk7W[(u7Oq7pxD<:A)et)ZF]r=7u)kDh/^.#%0*V+PZ]-As3hUKIk1gIyC{4x4ICo7L0hj=g&g:-hwo65pN{-zyTDs%O]aQ@Bj]w([G/rlAJ0g[/tFTX)"
    "24X2q]9^.^8dRwfJjemP/O(q}mhPB96qaQ3OQmirGa6>Ql)d?)P%rZs5ZSv!e*8&kXh$cZfsqI{&-(>^J<]p#2b}&WpzrZuw^fInMsI5E/^^0[}vj+B=Ha)$FtImXh!ACcB%4SgGOO>vTUnN+Xr.%rVcsJIgs#.="
    "?Q@#U?d%Iz<G<LU/aKL{@Kx@4=Zx8L/&l@ekuOb:Ioku^Sq6&g/L0C]3<JZe@@o9OWsXliDXWtD6%mNBoVSqdyi:!oQ[/60g*se%Lm/+^pz@^gLb?1#q!oAhmG#RR<}t17]^Q9I03C]:5>1F:ted-WmImRbrUK()"
    "&0mhchw:d(i42=0<#&[JI&RELai@s?wmbYP5-hI<5y8{Yy4G?qFN{0HN6E>%=Lm[]S#{dRxo1R1c=dol2eW/VVXF#yxxR2X<F9}:]*2eAWeY=%l5YzL5U@bX3J&qr5#srXi.I3LsG&MuY6f=xPJ-%nM[H.f*TUB<"
    "OBO+kaj=1N?DIT4IeoJ]Rx-$+@f^RJPDTWFUViifE+FjtAu.B*5n!8Pu$C4zS/h@g-XUNGZ>:ZCA&G9Q{eqL}W4!x->MJeE]LRqpiuqGHD^a$}^GcLzZ1-0Ev12vnwwg%HWj&)a6]N.wFE#pBFpCC5Uk{[st-??6"
    "3^x9C[/*Q%Q7BMfD]^Y*9^*3Ndh=V7YNHfcR=QN-{zGF3PO#cv)%-F?9e&Y1ZJC=$Q!hhW]%YPdn>w[7UzqGJ(v53^PG)AX.OGMHUp}uE3pi4d7%uv>)Gv7ORr/ZrzLzVXa}>y.1W0osZ3YO(A5m5yBiQ#w}Wl{y"
    "02Vr>*=>2v0M9fxjP/l0by<C3]uUISynZ^n/67zkL:F-)2.J<%@@3Se2}vdU9tfz00UmO%./(0kj0PfV81!+=(ALpguOz.a)=6wMQCLG=F$L12b9-!C75OB6OF:PFGx&rz+!Nz%XONmQj+%bzP!pS>hNpttCl3<W"
    "@Y9l}bF>E=z^+j8BrbKPDVNDUSrESvNVh&S5L]l}gawl}p[cyxedi9%B4]7jgMvFK7U<fh?]Z#Q=wp4wihK)%[DeYx+:dS(dubmb7Xk]m>DrNb+JvC*dA4wDprmIeRLuriI$(K@:Indpovin1(XPcW+!b5tk^aQw"
    "R)aOz<xvQ-!T)OUBH8dbjI!nVl>v1Y@gNVR4M8b9!EA-g5-+a-X>sT:9Q)(&l&*pRO}dSYN#2BQ@*(P{(%Dcpq.@w4nof85KTJ1AvU6NtS9{K8<=kz+{1v<rzR5O}@Us$XPVAj3kZ56[l!D<yX21Wtnr4u#aVZ.:"
    "jKw8zz%>n(I2RQR@ZugEYuOWT&y=5]0z/tP^+)8z)16tvew*6v8u&3ddB1dBH%TXA=w6KyfQkd8vt!y[]#zKPD.33%T97r>3XF#]BvM5E@r7Z+G(DmsDPnCw}G4W52h8GPh!-6eHAw:op%H!3ZwR8F+7)gb?H*5Y"
    "*S6pIiZ/Z!tdV<BpF9J}DkF:6X1&[48.}M@0bw-!sG^?-Z8XjxhN@*hHS6-lA:?HSxoD1TL?b@gMKaTb^oR7ZPwN}GwG]]KHYbSi1lE<ncJ?W)u&sAtm(&@C07%AP@vD(*}+VX8)@h}3*7xpZ!.c0/bUwu<A7-ZV"
    "-adjyMhPs:zpUFcC<UmWkMb(CY]UA^q0-JXGSw*z.m{8t^-cGI46bA-DL0bFnAb}.hyGwtIabf=Fx=iEOk}ZZ+J#vGZScDC724GOjiQxzzvGFK!&&9cq&%wQGp{AMii6Xmum)<bWT{Z^Y2dr..Bbsv+yhL{^/tf{"
    "Hnw+dbg]70gYs-T6e?dFr^VI>yQpol3zSaIXU?NphHGjsLofV7On^ia/SL<?)-Dk{2rdCp<-yNcE:Q#aK4/$WPV(uWFuHc-kiT!tZ/=]PC1Et-{>[&mn$448-jtqG!@QfDA0#6>emxqvGoNUO6[p:yF^f>u<LThQ"
    "YELA<sR}zmRn6fkz[GT08dlTY?:tbRZjJUkm/s*k0TkJOGTC#hqkf9LOkYhDCP[$R]G&j^D<1IyIY-(10oeCm>J!=t)ds!te}WOW0b)qb]<Th{(t+B3ly6TkJB/!A<?&df(EVQ=2()gP?M+hQ8#imo>6t=6W{QJ1"
    ">^:=/!^eo7o0FCbKs7:vC>Qmuti%Sl1*RKqV&N<Pll4lN?4#w(+[]*XwG$vLUB[]S@3w&R/72bOPIuKX6-%/wq5-xEMoGsW@HXMm+x!ln3{=Ou&1VJ6>=9TI8&SV4Hl.Z1U31l8GxO040KbwqkhZdp!4<c<UC0$$"
    "8]f!jlioHaN11{B4m9j8u0kMBrq?PvprB/t>Wh>G3!$}Kn8k4d{>8sl>%OVxx#Pvn3!{Z$m:u+]Xu6bIXzmF$*-mVPq8/t&S<PspORaT#c3h@.hEHg6g8&OI[px@RYsa+Y]J:lJmV>uzJ[RF$rzD4w>bz94*)ZL6"
    "BfDxE7NU%m{$E/x3=8[cE5[^&H.#{5v6z9Oz.rfa!tic9{J+$g=B)X?-h[p[]tMTYGhrAF2dJAGO!hpP0&:)RK<.<zY0oT*9Y<FF]-ko7[QvXw=A$c#MITkMEB60^eQO3d<ELf@hAW79.[:$K[&A??.14Nwlqk7p"
    "TKQ#55Rx7UNl?M+fV7A}w(dSfW(E&]Ve<(p.XWlmYlmK$hq+S$jP.+@8y+M+/V-&3fbSOqrl4b?4FQ*H<U@xM158Znc$qH{F%b:4u+@?F+@(<w!4U&N{&%!JPe%{bEM[?zv*Q54PvHc$Tcyu6B7xUT22Ga&FC[IX"
    "+2:b8^[^0877{?=<.RPWhxGYi!.9!b:mZQao}.b.Yl/E*12U=G9fn}zoC}nw+x9-A[.?EX81@^Z2a$[u1%7[ZH-3.XkZ?e}oaYZy][8b!/q0]l*jGxT?[S3)j66nT%lN${A6Bqgd<i-+SS$jD(*@PJ8V>n@Ge<ae"
    "vyMQ#E.C]c=ozowb#GEQ?>r>DW/eN(T-AU]Zbf-n?yrLzA<0@I=O&nU^mbzJ9n<^O8dmJwJWwfzK98sRxe4[(.!N9e9axnD4uu@/Fss]QzMd>9D[jBau9bK-.ZBN0?Vor?F1TsThQRDn!U^rim/#<CFCWs3G-cMQ"
    "ahp{Asvcz[)#3N[UScejGwl{?1e+09zrKp+Oe}q]oN=.M5JDB*H*Mt0y$IRvBILDh77/)nRBTKgK8qP%FaBD9>A9K]W={K:ULb>K?oGvUs:=1xfLPsWe!RWJ<tSxmSpY5fA6ueqbm[zp)Vo$lUFhz0V]W)CJXc@z"
    "E00U8w3X^A.OyZQu^{Kh9et9SoguU(3Nu+VS5i4+>wwAUtoo2aj47>G}%+E5np$L/WTlVt.Kxxt:Y+9/s(MpZKRFF-PG??dpPPK0))c{p)mv&ZU(<yzRHbEzPk^*nfHs^5Ffp=C-(J7&U^(0M8p(yd*L$2u2&+AG"
    "Lae&1jxx9/?=FMc{1lk3U.5l+aie*0MI+Yfqo7U<p>kOm2y7!lRX7K$FpULI^Sk(hh-!&q-eKF-xcW:u1.G#La*^]uIqRM^zsGQzVeuB!o(i21&UO4iGC0:F2xjas:S.%wjdCT9U*HJ-eqRUusDc1QL]Fp7.&q*E"
    "m0qn6lJK?akIL)^In[P)Q*r2URsU2(3kS=1N9l3=i09uKrD<+<)c2Y#c:<CXczy$RTwG8Y2@g>xo6JlXr>>Zp]nhmUYF^ZbM9[at-6<-evppq@Koh<xurun!3kj)u(sho]oxMCpn0i?:a*mZHJ5rbYO1IVKP-g{("
    "Lc$4R6SyukMiWD)DD{&0P:trmIxd4a1h@3ZNru&zFzs1t9rmjpA7)s?M*HvBaWeMF8S[rGKowH$pqZd*fm7L-bJ>aUf48N=hIF1exKvzP]@2iMqJ*.Z0]u36>Q>w=y/spfWnpOq3i8AA0?jZr/-1sI{4Cs/H(0NH"
    "h5<N.7pEY8V-zl24G5Ie[EK$3e$i9x{uO?[RTN5&M+UgpfM-4EOOqp4Ta1/-je&![cR?w$PJP)dbW9ZD2^!1D!?wMnEX!Hq]+{>f.z6))(EO1FO6ChUc&$&0NLc9YjRB--@FtJkW&mWU>]K7y5.k+i3k8nwj}IS)"
    "nKE]$@sh9[HlwyVUAR3!kRs7h5?1mH.K2ImnNwUn[gnrAxd9xSxp?BBj&/+zSK1fvt9-8$}V}^*JuAm}?i2^XHW4e9&r=tv.}Ed+:9Jf?}}dy7i+wf7md$s-G.(j<V+djm<&du.9^fjEdp9$m1+-/4stLLJd9$@e"
    "Qp1.juuXGGZgI@Wc#<:$sod%S4O^uY6<Sl5s&y3Z0fv=]PukeDg}OBa(y)1=J55]j=h]<qyY50:/]+Vfka@VA*qV%902k)bwZoE+Ne+l$zk<Bihw-EhUA+5*UR15$W@(qk*M@<{}mfB^6/{rPaTVuK]+Jq9wTKmN"
    "fpDt[8dUM43%#^e>(G9taQ%vRY/29-.nC3jmx^bZWQIFi^*whA3Kl7qqxfjx&sDvg/krc$g0afrKM8jI8D=v)&JJMFB/Nvown}Emb#z}LO(:MY7^dXR&9rTLRSU*YG6HlFt2NJlg73u.UC08KTD1I(ZrpBNjDZ<5"
    "PI+c-vQi@A)#kafIoW]k?6UdJ0ywp)P=:I&*7*7(h.gvlFgQp4cO62ynRG2#wN&ZTr^DQPhUv$B*=(Z8N*+T=}^K.rB3$+lL%K^@5ye=%N=@?=0e2YxhTt0&KB]{r1-m<f:pgYBK{/P*{+):CX13.UyJFM}n=#tt"
    "Rv?/yY9$v+-Dh]379U)b?3xWXMGr$AO$znCggVN3DG=W$@:.s*^s-h7pOKZf<rl1H=DgVR6kLB<y]N2YMW5-CaIt&[7KjSGbcIJu}6?(sGcSC{06*<DxsLi<F$pTu27+kw8Xl/(bM<0769V5N?3z7o^7@hNr(S1s"
    "&1bPS*/%HpaOEux>B:RWO.m(tdNvcrCGb7y]Oy{r:Y[bq?r{L/.+a!AcOnrlT3UBpo/MVbv/]*m6B@ozDzOSjfOd1O)1Vr0lXukPRe0)lM<m2J>%HuS0aAG4CmJk<>du>G8g$O}EX3%A^3Y:?o>v$&?Z90Ouo9cg"
    "!lc08GOpAuKI{6GkNyLJCv##8!Z}E&0xEP6R]9n)IF&5Sqj.tV5nOyiB)FF09nAQGP7M*!9%A6r9<^FD}aI+y&.7gf:^%NZomG#*t]!)VyP]JM9+=R{}dHzwLRSNh2/BN>*.h^F7o@xHav^L54O.n&Jd}SHm-30)"
    "Gz16a{R(b!w=))^<Y}7bA0QecAEqxXCPfu-Oqdlo?AP/tGuN%UJw1<UwQ/:>=Sxz&E-MX]!!*hXaoDHc1njJxAL8WbySYF3yrI<o!vJKI&!3IXVC-dgB%9lS1dO]=v[}B}tAB)n}lCaRV0asL/3#2%x=8{tHeN2/"
    "v>BJO[3hiuHT>BRr5BAO3qYq?37&tMF[dO39l#wz=]Dyj/)j-(J5SxG30]=SJ.W5K?>Apw(<O=L!S#/U*<0aE50)??VguNi*8xZxt+VheRM:It946JNWW}Kr(DaUo18]*X(D^&OL%tW?w.]-rp%5b>pmDHr0xs#>"
    "su)9?H35Gog{F$pZxI+RISAD)9/#4SjX4Xfh).[y<dK&mgCrX{+hNM&Hxz*QUM6O@1}IU)^%.+B+2)8)]%Rt3-&dJ9nXK#cv(6kXlVr:(Z/5g&ONcczk@Kqm0CW:UTAge+W):W>fD([#]HWA.^.{3wH^!f-CAX&c"
    "v*o4#KWNBZofm%hd[6j*Zy6?Y)JFrw=z9x?0aB^t8*-%rDYiw3t{oRC:5<4aFOD$]<yDz4yZbdXCdQk@)3GN{(LlyACN:8B(YBc0aR&3-1cK?[nS4T0wrl2T<6i-yD@+N>3>4vaylo*9(:4ruicP#%&AmHRz#+p&"
    "@#+%D?DnSDkIQ<#bm7Ao[BeVH4%30Em4nuhB8^ZD(Ap)VgWH8^a=N1>d{OL0]Xzc1/MEg>@fwyL&-4lN+8z?p{L+n:a.T2QJ>I?Q.wb4P5$34mWhk[ZDX+u8n83uTx6qpct^6&Mi(/Kw2N:DeRu8?YfVjyMVecm:"
    "u6q5)C2A[z:XkMzeJOCl.P-^lo0H[hKSuL^^md.N:^1>jm{3+EvHRrk6A&*[]G3#7ep6[0)9rDVQjV[NwRMI6FeN(cLvX!qx>dlIu+&7Q88tYfV0&rR!=GNMKn^d81FQj:gvVM<ItN<(G81HEUMROo8lD>d00X)p"
    "Ouo3vE3k:[=]P>9lGUr?bxznX8>JegLA$zkJxV1Zpu}44D(rBDGI:7vG&R9$a=Q/+&&2ms.4&6XKomQd8R$>0aU>nP{*v&mxkO/lj3AY&+QG3LLZvTvWH>K4fZA2v55Kk{OQl]tx*z&II+GIVb:[]tHm?R]F]wfg"
    "?^6s+&co9yb2y:*tFqsz)X0(N1?q2!BSB[(!lE7ddDhf?4M2BOGD8G*Y6>OaM9?*5/p]gt9/ZA:Pu}}lxtY49{&bu$/bzE#Uw3WP)$$%&NjOH.Wn=rieyt[)WALvNo?z&ljGRKMQ4Uz?DG/jerd0TJi^2b]@Ak@T"
    "bPhw%A]aJ-]7FweTgq9XyZ{C]a8-J^+-CA8)sFJc)A-d3Kz>1u!650srj:@XipwUkx!+]w&ViPG1VDBB^%Ao=MrzMIda%-Ei9cE76*M-OExm1{unk+SRM:dWh!9Mc2xZjGRz]nltb28eT(.]+)K1eMt*)HE@h?>}"
    "y6bgWsqF1W5&IaRCX>XGK4+iWwz6nL)9(QgNzGa.m=%*b1j6o}aaMimFF?H^<^65b3b!s[LjJ#8Nw-!nY-TUr{*w^>YR!EY3&Ik2ISN[y}Ae[c=uCzkNXPX5pvYb(C0U}T:o8%&k)36A?lQYcil6q{3Xl}x)JDiE"
    "yRm3M.9$D[pioR#G^0gHP)k<<1rw1AG6ogM]3?OyC$jRKW68s/CZMzHK50W1(a$kp2y3>z3/ncd0GPLkGuDX3hXxVr&z&WMzxMF$Hg.Yq=ZA{?CyFE^h}Emy{rlYFKt%EBTAXzXZ@vpayhxvH/JM>Vcycb(5%R%="
    "HQ&ZMQm=uFiFIg)KWz:xpU>nwH1lrW0PegYIc.Q82#gY6?ILjsu%?Fr)6>=iZQ=1ha}aHc]o04w:n$Wv9:v<1uM+b%&O!i6B@-O69g7=B!9@J=}iUpHYfi*ABq&[}ILn==MkDoqc=Nr?rsBko((44Bb7!.SLsH^D"
    "/{5m#KEgcA5NfKha{T1#tpCsfz#?[{]i[KaL]NUG7G?r(^&.1sn-(kwMpJ9<AfdRb83kw16t/(xFhOxb3D}&*kYx]7<t!woive3XLQOI%s4pi0m$lqfov.T)fQPKlq(=u2D(.>kBUGIriX^zEl]8My^=2esGK$O."
    "9jboKV*8UEu9Lf5u-qG$G]sBBLo(WdEI=jkqPLSm7p5L:qE[b%FG)c?cMTn3PRROkVvgq*P.yP%Qc0dRKC1@D<Amsz=eVXXME{h3F7P3z:UjrvW*25y11kdU75FtHTEL<5e](be!1wSZb*kI65qZiuZK@6&y:xZM"
    "+**sJH9c-9:PyS*y0fUdd8HK.2QFfeqg&I=/I<&R/5d$$/^KJSN{Bxy9:6lJE>%<4!)V{T15x9$GR%FeF[:>zhi#?LaWG6^R2c0x2hPCnptO!Ia>HCYTumv/CJQAcl29*jVg0i[u@Gs?yRvM?WJe]!H[n4&IT/lb"
    "VkA9Xvc=s5F41t.]R0P/+DcNRA%t3m1H%05(&@R@Z-*CY.5$-pjBu44G>JcbcfC:b8oaT>-md$w@e>GSqbUY/u=ZWhFgORSNMhF%Gk(fL@J]RmUg2<EWK-5$y!1nmndC6dK%acG=zZFSYcofqU^^E(]ek&UFEaLj"
    "88O[H80SJfk/2icTpV)AWHdEU^H5XMA0HQ)O/:4G@LB@YfFBbW60o>&XKA(J?/zaJVEk>K/m!-vp/O0ND/z#/5S=BsW6ab=Du>}UN7X7qNCPv(b/+S4-/W(bJN.bmpdUUG0u85![qH]ysZYy@FDkD]fHXi#SQ(}p"
    "7C<na([lu6w#v]8pMhre6:(o+1Kd]96f=/KV4D=-DzOYa+*t$95k!Iez>FFq9g>(1BnhMb0I(c+P<m8{)vWR3VXfKFCRa30z1AT0B+5[JE<E3Dxhs%0h:KfC!N7N}mzdBpu(c7K=E*FK-AJ5)J&g#HC%j?f[sOL{"
    "]xl#iPW2UPhfe]OwAF{kaaX0I4AH/c1ER9y3x]Kc}Z(>MGQ6Nu.%Uto.bQHWb7Ul(?cTs3k%vZODE/G>SK!t#:?)lnr-}/{=.vWb?1MdJcLcOnEJScN@yGUIUVIW<8)RQ(GY/P^-LTCI=6*N/*l1<IO{}1G9D7:E"
    "^8:Pt9Nrf-K&6R)oO@fD!FZEBqk?vrit1p7FSYFKpM#aUHOY97R!4WPlP0pm<4w{l[v1>hd%XZEVB4-XjVH/y-7gtpJIaASH*leJh!jZsYx=>G3Hgwo(20*Ov6!?$+Xz7(cc3k]7F*=MkX0Jlq(+]Cr$%r[zjT2o"
    "j+qz[^.-/dhihWq3SDKvvCoP+]l8S1CGsHt-^6ubiS{G{[LHj<m!vd:GevVoB[bpHoZG$sSzfP@p{sXTNk]h5zPKPDd[pY2Yu3MCw0hj>Lnrqqyy{{LHI9Dm&V>ZImmG]2q!$2UQu8/dLV6.pB?qNF:{h=FyA*qe"
    "QW]&]bgDtu{/B5XGOF98sHY#Zc<IgM{4olZH.Ue5:GQx$]dmTiArC:wh<$dVcuB&:rtFT!)990qP3/2F7.h0vhrtX!XfaTb:^+LIGMdC(6)LRRq*8KCNZD?Wjc<6i36PEF=oe!dK0uj1v[)1M1pXhFN>KQZ0^/2n"
    "3)w[8p^zj4[ae=w&cmC&Y+8hRKN]2^ao*$..^$&%0IzrclwG5M(s7OKOCW#VA}fAcbx9bK0nsA<s]/.3jyDl{N9AI:*<]]]?d!Iy=iOasQ!H?E&OZ(LMZgzaXk9^61LfPF2]0Qa/#N)G>2]:g@v7h+ouqj.9p6v/"
    "pesH6H+/q$0f?#s5s)Un/x59:Gx:vd.2AWuQ.Z4OT0c5[9Fd=56[r}1UKHE1{?[b:kX>>F9^kPBO.n/(<Rx#0i/Ozd6W9EDH>45T!#ut!+dh-!Y6<%&qP]fi?Tx3^MFL9Bo<U.ocCpF-zn#Zl/t$BHZF#I^x3v>!"
    ":hQ4GLITyav=fJ?H?fEev0(FTx0CH)xvGQA-n@HtB?b>b{lDyNnL{$WP:8]RuwAL&IwQ>U<5QECKWUi=Q9?Rj4nd:XTJ&({]nd5IF%W+8fZxoxgQHsY0a+EbQcva*&Ry8xC&@S[3@hMH63g7heJ^{/ep[9-*ASMN"
    "4iVnM=z^%AN{x}<y<*u[(=.+ZilmQ@BB5ZP(-z^%>xaMJ:UO9*z}@c2u@(i!Fq1/<KtBpRvsp2=eOQIFtK@[y5uQ9QBmRIs89c%p:MCC3b8zRp8Lh(i3?mwOvy5{rLUt&s(!igx905fm1cqNB-bYCiI!{EPi^$R6"
    "74zJvB<A#W&$gNAfl08c4FHvT72u$?+rVP9U!k[8d0qp8}wI&0IPucmvKV&?(H%C6/JJCHRsBVnVNzeT]v#ZNt*9weG2t)(JF](8Z^%[]qMWo4{a{-mm)z&Jn!O&C5pSg)@l6x<t@=ReE(lazQ3jKm-RB(/+PIPG"
    "kRGG%56D7MhuD[&ujLew6Y-FXGmQn0I^X8XWUT1li$tV<jJbxj498jSatVz3QQGkIhgM8N98wjR2>34o2Zb[Hs3ERYBH@]D[w{-&Khg&Cd<vMn0cncuxAVfEL)6@K*.o}@DC^DxCz7b^Wpme.wt*:QR2XXJdp60f"
    "*%jts^D-U82<!FW(C9JOV/lqeCH<-P&7P9>^mE}<ZU*vzh>ouv(LH)h68d{Pwwe(CG[ktI!)Mw<U!0T)pzF.%n]Zz&sCNLBd)qpXLFH.^{%g=4jDmw#+*$sgCSD-k^26dSw})tRM2+C@UM)FrglAMF=%GlLgrrrl"
    "*&5Tb7Y%CUyEsHOK%5k>qzJ(o{k8u@*?3Pf!/sR?<yP.QpGns9l{hsS0TzrIwlgpm&iqRy0vjiieX@qs58Ce2a{bv=(@9Kyaxg%E1?.G5R9/m)fFSpFFLCcMr.?6s3:IjM87t2}>NoF%<(i[j:Q4m(ZZsEK=mx7Q"
    "jM>7KFps47yMZk/3):I6m^9u5Lo8q>Yx#d].0#*PGAdTUABK>&iI))+enH7{fWL7x8-{c3lF{{C)L[y(3k[g!u</O4RqZr.dV#Vbfj$#%D3Qs*URANy5gXkRZZE:QcYWG$Obm6GMQU@1H-lqSvxy{9Pcf3ctxwG+"
    "Ih*NZnoJ0ZBLP0:A16bju(V=?MizdKs8xv!c$U-p>-.0*4tEWW?rHU^fpIt(<%M{V9:w!@/GA$cz$a7=EHFueN$):FA?jL-if#yIPF-vWA6$FsTD<^:GFW&DT1OQWrXpql:X]Q6fesa^x}imzIS@Y6iABBn.k%&Z"
    "f{Kqj@mzx]w#n#C.d]n[Q+G$<MG!Gq2fE6p.%>4zC%lg//fz#Qx@w^oZ)m%Tdz=UMwcbO@nSkf=elX-uK!<.138B5&FObX*gfF4Sfnic#K$rN+Zj0um*jz34aR<{P!I%1BGqXU!2rd3(:vt?1+hoWIT]KOZ&x=5h"
    "}SVp7r2ct}iowdcwA%s}ukqfV[@EPPjyU9#O?Vt8c2@MYwFjfVQ=VXZ6NR5(:GI2ZBcp5Zgi8x0p9AuPocrVo^%V$Kmk&hik5ME*]QXkg@^=RV:Gr@!niudYZ+1Y7r>%y$tmzGw:eOy7l4cWJs<b*soz:rrEC+z="
    "bo[3/dBd+0ax6*Hic31AbgR.OC:F8=lY<{Mk>m2WPbOUfDJV(EhZ}2)Y]ZJ8VD%R5yb{)98g$5%5U5X(Sz1a2jGuYbp=2rH-<.Iy@3S{hxp+^*)x>b=.htQ/ct)hG!w.+aQ.z7&}Sl=L12-@]>Co<%rH9]x?TgQ&"
    ":&F]oi7*w[!>YgAn.]YM7o9n[LGSGAk5ls<aeCGx2tO[FRqfYVvQA*(X6G%VuAg1%}Af2N@/rHsR6Vz&AR/hA3{kk(%ijShm-(CRe#ZTS=)Rrfvzhjrl9DSsqo[u)<1Je)lyap.<t<yIp*OAE%b!eWrwXNL5H@Xc"
    "C[Qe0&Etr/)ne7:pIM5n^+vq1V?l!)akvD!JgpQd?%k@/Oe6vM8-wHO=?C[4!5=(-i0!&BfBy0X<<o[t&ws4@H?8z!2=6OhkSfapHQOH[ZP^*bMMZv0/FrAwQVybM?%:RIYOHU3Gl#+RXO.vmcKAp7wNOJg<S!Tg"
    "Uy1Mx^BKb}t$]BIN!WNgxj:4YK/)G#UmPHETe]C&UK?w*2Bl1i5ZI<6jkbs-(s{ljuoc#%*%Q0xG{e(E8B[-+NAeb0zS0FhKHP&j[cbh}%lAu3<5)df!^fN+ZVB%klxjp/5s1xC)8hLP/>h7r7QJ6#jt2k#iIWUg"
    "PUAAbv<=YAM*@KI9eB{5oGj8qT]zLUF5d5S>lZiWR!-A-e6!)(UKKXYa{9:6E7PHa?JlP3}U#W.&}q&=ar2HYnkmtE4)R*A{AOM^.r^--}*FD(lnwU[g=4{i8y+IV:>Xzp)(7<-n:jH3fkSGA8tcZ:o/>N>Ym.T6"
    "]B][0O2.$A7EPHNd=<Q?+aJAXhy)Z<UCQi(v7TNfWLD#8c.wc/9(u25=(NwS(?]Q*U.%dV&]zdT+H$(Dw!QF1oKnA65n?mvqvmBulm87j1CJ$IdW<O#u@Q/+x{u?T!MrZ60H%bVGlfy=W9>:HJ5yNH%m8-$.IJik"
    "ds8>%lO^btixbkHMxSSmY#{#z8S9BM^q21/cxq!XL146<wY*f%cWd%Lv@<{kR${6e^^Qcuu>WjkIIJBjl!(@pRxQXEfux&2&@E9/@df*oe@.8XO4Su!VfTg!mCtJyB<tn-2+B1:ORuJ+P@d1Mx3LAyg6Sv]q0-G6"
    "[4/os<(W]5(aBX0lF.{2ZWU2mglM$o90Ci$1ZE0RDeN70fsauO@AYnbelRJ0dstpq-nae8rk1leLBA@<)B[ow9cq(5.*e2vV6dgvksZ!IH9&yX^>ZA(/$N%XF3y%-G9x[HWI&3BbL@S0n4%}rZ%mHQ!^z/)ofx1R"
    "C$%w4rLP:[fqT#SI&XzN891B6k$TdmM={=0y]*YJRikUzIMCESUGK67sp>-T&nJDkjCQ^JQgMz%q]C2?ux3R!.Iy^NF@CRKuDX0zT!1J2:zX4MIlvn=Lko<TVN$}{jXL*s4W8{umMoxFtx1Z5Wb*EX(ds4GR><]9"
    "wkzvH!L595:(t/LxuohVD)F}k0lo47&y{M&uDPFvLSA)uI^K4En)YW}+yC>pG2miJr0YoG?^Mbkzl9Yd7ViyG{b>l5&yhhjxxqBD&anuV77C0t&Ru{/yhNyB7Yb=Dc!IYmr>pCd}{LGN87c#ki-[M)w]+ZL>ki9{"
    "HFbVkZH@4$fNiL<q!Hycqr0J$ZIR4k>Y[%ibiuDsjI{eFUq0cI-0F13684Wdu]w%>fy-4GaK.0}[B$*2kx9m-amC8tU-Vk7<6l8Q/c>j?ei+6o4s4wN8kFE][K$f:n43f<ig?O0tirfM]/$6+[soVK+gK3Z6A6I/"
    "ZTFIyrQ-9IN7Y<RC+J8:^X#V-Vx+n6O%QXtU)@^bK.6m]FPgb:^acrYSq[d(6gmrs7*l66p*NsKiKkz}<y%{M@pap<+CmT(b9cnD&mqgK/4O)R}Z!<mu:D]@BBlOWRe=Q!<U(Z$H2#?7DfEsWn%DP[n-2qF>T1&H"
    "alE>N-Uc7rW{Ay20#}f1U[9m^7Ovx)Ifr2M=0hsat#:ZQPlDOa8:EHgS3-84:[PG-KhKL^-Eu5>QLdC{}a001(v5URpM*Wnpo[X@>/O/JkiJcF0/KJcA7+*$(>lp]QkJqbW5s$us08P}DiTG/0C3FZ63)lSrB&.a"
    "k3Gs^CR>C:CWSU27p7%.k5+]qLF(ZIiFdLm+*Ra]jOcqd>tzTShSmVyEJ4nM4m/dQ55=(+*I9%v^]dKg:SR^}P#@pZsOdlfbD#Vp}bn/4E@CK%H{#0W>7z59!BdQ}S&p/NVG8=XyhDCz!-560&T7Mb:-ZC5l6/w7"
    "/%gx7c=}izFfCE8bo[#B%m=(x-r!H%Y1E^R]nLj$[W3AtHwliF&SitvB1iPaeZoky>NVC{ngw}py?1kWWYAZqD7c4hKABwWermvwN3WyXgV6PYG!5ma[A0EV*7DK)Tfvrb[!9JVE>6v8g8q<&Y$=mcYQ7iT&uNm5"
    "P23i0D?5oG=CLnWGP-[b/2%916HhdE@qqPf/5Y)-BAONPc.wyb(2>k1OoZDWK37cEJWm1]w!-ChhDj:ZU@iUS<fU!KCl=vX%cS&R<hZZRTL!<J(uwP}kB!KpBEcwG&Z2F9:=+Z197ZGD-g2O*LGRw(LyfpKu3qUB"
    "6pn$E4XaMHB*Q}X7ho9<%i5gP{>OGZ(ITkN{Qm-}>z9gqpbdy!Ew=mHJ=JlpBbVn}<LK?l<tQd7I($#+3XM9l0GUM!92F&4?woQdA$PGR%k5e2HGE/:l7:NhtMna4Sl#8!3[}pUeuRurGhvd?Nu>sW=g=CD:csO{"
    "nwAUnl}U-!lTG%u^O:R(e%3:x-VAt=zAQOo&uGx17IE:Tb^S1rqfu5Ysi.9jcdgRXlvc&Lt:H/E=9{MPQ-NWr!J?&Gi8#Cy(0?=F-DwzJ8aF]KZYcE}?lR?pX8qnBg7a&b+s91+:5/pnE*y-l]>?.w0y3J@dlLn+"
    "5Dl^H?:hop)wbE3.=e&:4T^vq2I{}=&@x5V";
// clang-format on
}
