# cpp-mython
### Описание проекта:
Программная реализация интерпретатора языка Mython. Интерпретатор получает набор инструкций на языке Mython в поток ввода и результат выполнения программы отправляет в поток вывода.

### Пример:
* Ввод:
   ```python
   class Shape:
     def __str__():
       return "Shape"
   class Rect(Shape):
     def __init__(w, h):
       self.w = w
       self.h = h
     def __str__():
       return "Rect(" + str(self.w) + 'x' + str(self.h) + ')'
   class Circle(Shape):
     def __init__(r):
       self.r = r
     def __str__():
       return 'Circle(' + str(self.r) + ')'
   class Triangle(Shape):
     def __init__(a, b, c):
       self.ok = a + b > c and a + c > b and b + c > a
       if (self.ok):
         self.a = a
         self.b = b
         self.c = c
     def __str__():
       if self.ok:
         return 'Triangle(' + str(self.a) + ', ' + str(self.b) + ', ' + str(self.c) + ')'
       else:
         return 'Wrong triangle'
   r = Rect(10, 20)
   c = Circle(52)
   t1 = Triangle(3, 4, 5)
   t2 = Triangle(125, 1, 2)
   print r, c, t1, t2
   ```
* Вывод:
   фывфыв
