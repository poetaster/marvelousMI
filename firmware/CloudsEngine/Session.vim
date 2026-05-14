let SessionLoad = 1
if &cp | set nocp | endif
let s:so_save = &g:so | let s:siso_save = &g:siso | setg so=0 siso=0 | setl so=-1 siso=-1
let v:this_session=expand("<sfile>:p")
silent only
silent tabonly
cd ~
if expand('%') == '' && !&modified && line('$') <= 1 && getline(1) == ''
  let s:wipebuf = bufnr('%')
endif
let s:shortmess_save = &shortmess
if &shortmess =~ 'A'
  set shortmess=aoOA
else
  set shortmess=aoO
endif
badd +0 src/marvelousMI/firmware/CloudsEngine/CloudsEngine.ino
badd +0 src/marvelousMI/firmware/CloudsEngine/samples.h
badd +0 src/marvelousMI/firmware/CloudsEngine/sampledefs.h
argglobal
%argdel
set stal=2
tabnew +setlocal\ bufhidden=wipe
tabnew +setlocal\ bufhidden=wipe
tabrewind
edit src/marvelousMI/firmware/CloudsEngine/CloudsEngine.ino
argglobal
setlocal fdm=syntax
setlocal fde=0
setlocal fmr={{{,}}}
setlocal fdi=#
setlocal fdl=0
setlocal fml=1
setlocal fdn=20
setlocal fen
30
normal! zo
164
normal! zo
167
normal! zo
168
normal! zo
196
normal! zo
234
normal! zo
280
normal! zo
281
normal! zo
358
normal! zo
360
normal! zo
361
normal! zo
370
normal! zo
373
normal! zo
397
normal! zo
409
normal! zo
411
normal! zo
438
normal! zo
439
normal! zo
448
normal! zo
454
normal! zo
467
normal! zo
484
normal! zo
497
normal! zo
let s:l = 77 - ((6 * winheight(0) + 25) / 51)
if s:l < 1 | let s:l = 1 | endif
keepjumps exe s:l
normal! zt
keepjumps 77
normal! 0
lcd ~/src/marvelousMI/firmware/CloudsEngine
tabnext
edit ~/src/marvelousMI/firmware/CloudsEngine/sampledefs.h
argglobal
balt ~/src/marvelousMI/firmware/CloudsEngine/CloudsEngine.ino
setlocal fdm=syntax
setlocal fde=0
setlocal fmr={{{,}}}
setlocal fdi=#
setlocal fdl=0
setlocal fml=1
setlocal fdn=20
setlocal fen
3
normal! zo
let s:l = 33 - ((32 * winheight(0) + 25) / 51)
if s:l < 1 | let s:l = 1 | endif
keepjumps exe s:l
normal! zt
keepjumps 33
normal! 0
lcd ~/src/marvelousMI/firmware/CloudsEngine
tabnext
edit ~/src/marvelousMI/firmware/CloudsEngine/samples.h
argglobal
balt ~/src/marvelousMI/firmware/CloudsEngine/CloudsEngine.ino
setlocal fdm=syntax
setlocal fde=0
setlocal fmr={{{,}}}
setlocal fdi=#
setlocal fdl=0
setlocal fml=1
setlocal fdn=20
setlocal fen
let s:l = 4 - ((3 * winheight(0) + 25) / 51)
if s:l < 1 | let s:l = 1 | endif
keepjumps exe s:l
normal! zt
keepjumps 4
normal! 023|
lcd ~/src/marvelousMI/firmware/CloudsEngine
tabnext 2
set stal=1
if exists('s:wipebuf') && len(win_findbuf(s:wipebuf)) == 0
  silent exe 'bwipe ' . s:wipebuf
endif
unlet! s:wipebuf
set winheight=1 winwidth=20
let &shortmess = s:shortmess_save
let s:sx = expand("<sfile>:p:r")."x.vim"
if filereadable(s:sx)
  exe "source " . fnameescape(s:sx)
endif
let &g:so = s:so_save | let &g:siso = s:siso_save
doautoall SessionLoadPost
unlet SessionLoad
" vim: set ft=vim :
