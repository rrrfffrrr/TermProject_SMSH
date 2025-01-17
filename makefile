# @author Kang chan yeong(rrrfffrrr@hanyang.ac.kr)
# @Create 05/22/2020
# @Last_update 06/21/2020
CC		:= gcc
CFLAG	:= -g -Wall -Werror
TARGET	:= smsh
SRCDIR	:= src
SOURCE	:= $(wildcard $(SRCDIR)/*.c)
OBJDIR	:= obj
OBJECT	:= $(addprefix $(OBJDIR)/,$(notdir $(SOURCE:.c=.o)))

.PHONY:	all clean

all: $(TARGET)

clean:
	$(RM) -rf $(OBJDIR)
	$(RM) $(TARGET)

$(TARGET): $(OBJECT)
	$(CC) $(CFLAG) -o $@.out $^

$(OBJDIR)/%.o: $(SRCDIR)/%.c
	mkdir -p $(OBJDIR)
	$(CC) $(CFLAG) -c -o $@ $^
