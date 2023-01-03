# min-tar-arhivator

* **compile:**
```
  gcc -Wall -o mtar main.c
```
* **run:**
  - *arhivare:*
   ``` 
    ./mtar -c <archive_name> <file_names...>
    ```
     *__example__: ./mtar -c arch.tar doc1 doc2 doc3* 
   
  - *dezarhivare:*
   ```
    ./mtar -x <archive_name>
    ```
     *__example__: ./mtar -x arch.tar*
