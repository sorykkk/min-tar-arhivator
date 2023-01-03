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
   - example: ./mtar -c arch.tar doc1 doc2 doc3 * 
   
  - *dezarhivare:*
   ```
    ./mtar -x <archive_name>
    ```
  - *example: ./mtar -x arch.tar*
