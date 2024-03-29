NAME		=	colorart

SRCS		=	colorart.c \
				analyse.c \
				colorset.c \
				color.c

OBJS		=	$(SRCS:.c=.o)

RM		=	rm -f

CP		=	cp -f

INSTALLPATH	=	~/bin/

CFLAGS		=	`pkg-config --cflags MagickWand` \
				-O2
#				-O0 -g 

LDFLAGS		=	`pkg-config --libs MagickWand`

VALGRIND		= valgrind

VALGRINDOPTS	= --leak-check=full


$(NAME)	:	$(OBJS)
			$(CC) -o $(NAME) $(OBJS) $(LDFLAGS)

all	:	$(NAME)

clean	:
			$(RM) $(OBJS)
			$(RM) $(NAME)

fclean	:	clean
			$(RM) *\~

re	:	fclean all

install	:	all
			$(CP) $(NAME) $(INSTALLPATH)

leakcheck: all
	$(VALGRIND) $(VALGRINDOPTS) ./$(NAME) -F 'background "%b", primary "%p", secondary "%s", detail "%d", percent "%%"' 'image.jpg'


