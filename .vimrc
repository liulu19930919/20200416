"
set nu

"
set tags=./.tags;,.tags

"
colorscheme desert

"
let mapleader=","
nmap LB 0
nmap LE $
vnoremap <Leader>y "+y
nmap <Leader>p "+p

"
filetype on
filetype plugin on

"
autocmd BufWritePost $MYVIMRC source $MYVIMRC
call plug#begin('~/.vim/plugged')
Plug 'universal-ctags/ctags'
call plug#end()

"
set guifont=YaHei\ Consolas\ Hybrid\ 11.5
set background=dark
colorscheme industry
"colorscheme pablo
"colorscheme murphy
"colorscheme morning
"colorscheme peachpuff
"colorscheme evening
"colorscheme blue
"colorscheme darkblue
"colorscheme default
"colorscheme desert
"colorscheme delek
"colorscheme elflord
"colorscheme koehler
"colorscheme ron
"colorscheme slate
"colorscheme zellner
"colorscheme shine
"colorscheme torte

"
set nocompatible
set wildmenu
set incsearch

"
"set gcr=a:block-blinkon0

set guioptions-=l
set guioptions-=L
set guioptions-=r
set guioptions=-R
set guioptions-=m
set guioptions-=T

"
set laststatus=2
set ruler
set cursorline
"set cursorcolumn
set hlsearch
