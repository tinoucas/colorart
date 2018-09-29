NAME		=	colorart

SRCS		=	colorart.c

OBJS		=	$(SRCS:.c=.o)

RM		=	rm -f

CP		=	cp -f

INSTALLPATH	=	~/bin/

CFLAGS		=	-O0 -g `pkg-config --cflags MagickWand`

LDFLAGS		=	`pkg-config --libs MagickWand`


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



