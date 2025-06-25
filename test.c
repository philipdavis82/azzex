

#include <var_buffer_2.h>

#ifndef TEST_RECORD_LENGTH
    #define TEST_RECORD_LENGTH 100 // Default number of records to test
#endif

#ifndef TEST_EXTRA_VARS
    #define TEST_EXTRA_VARS 0 // Default number of extra variables to test
#endif



int main(int, char **){

    /* Example
    vb2_init(); // Initialize the variable buffer system
    vb2_open("test.vb2"); // Open the variable buffer file
    
    // In the giant for loop 
    // There should be all the information there
    for {for {for {
        char *typstr;
        char *name;
        void *variable;
        if(typestr == "int" || typestr == "enum"){
            (double) *(int *)variable;
            vb2_add_variable(name,"nan","nan", "int", variable, sizeof(int));
        }
        else if(typestr == "double"){
            *(double *)variable;
            vb2_add_variable(name,"nan","nan", "double", variable, sizeof(double));
        }
    }}}

    
    vb2_start(/Number Of loops/ (100000+100));
    
    do {
        // DO STUFF HERE
        vb2_record_all(); // Record all tracked variables
    } while (running);

    vb2_end(); // End the recording session
    vb2_close(); // Close the variable buffer file and free resources
    */



    vb2_init(); // Initialize the variable buffer system
    printf("Variable Buffer 2 Test Length = %d\n", TEST_RECORD_LENGTH);

    if (vb2_open("test.vb2") != 0) {
        printf("Failed to open variable buffer file.\n");
        return -1; // Failed to open the variable buffer file
    }

    // Variables MUST be available in the scope of the recording
    // They will be recorded by reference, so they must not go out of scope
    // and must not be modified in a way that changes their address.
    int   var1  = 42;
    float var2  = 3.14f;
    double var3 = 2.718281828459045;

   
    // Track the variables with their metadata
    // The type is a string that describes the type of the variable
    // Current Available types are "int", "float", "double", "long"
    // Example without using vb2_track_variable macro

    

    vb2_add_variable("var1", "units", "description", "int", (void *)(&var1), sizeof(int)); // int
    vb2_track_variable(&var2, "var2", "units", "description", VB2_FLOAT); // float
    vb2_track_variable(&var3, "var3", "units", "description", VB2_DOUBLE); // double
    
    #if TEST_EXTRA_VARS 
    double LotsOVars[TEST_EXTRA_VARS]; // Example of an array variable
    for(int i = 0; i < TEST_EXTRA_VARS; i++) {
        char name[32];
        snprintf(name, sizeof(name), "LotsOVars[%d]", i);
        vb2_add_variable(name , "units", "description", VB2_DOUBLE, &LotsOVars[i], sizeof(LotsOVars[i]));
    }
    #endif

    // Start the recording session
    vb2_start(TEST_RECORD_LENGTH); // Start recording with a maximum history of 100
    vb2_record_all(); // Record all tracked variables
    for(int i = 0; i < TEST_RECORD_LENGTH; i++) {
        var1 += i; // Modify the variable to record
        var2 += 0.1f * i; // Modify the variable to record
        var3 += 0.01 * i; // Modify the variable to record
        vb2_record_all(); // Record all tracked variables
    }
    vb2_end(); // End the recording session


    {
        #ifndef TEST_NO_EXTRA_FILE
        // ===============================================================
        // second session to record to another file
        // Note: this is just an example to show that you can record to multiple files
        // ===============================================================
        if (vb2_open("test2.vb2") != 0) {
            printf("Failed to open variable buffer file.\n");
            return -1; // Failed to open the variable buffer file
        }

        vb2_start(TEST_RECORD_LENGTH); // Start recording with a maximum history of 100
        var1 = 0; // Reset the variable to record
        var2 = 0; // Reset the variable to record
        var3 = 0; // Reset the variable to record
        vb2_record_all(); // Record all tracked variables
        for(int i = 0; i < TEST_RECORD_LENGTH; i++) {
            var1 -= i; // Modify the variable to record
            var2 -= 0.1f * i; // Modify the variable to record
            var3 -= 0.01 * i; // Modify the variable to record
            vb2_record_all(); // Record all tracked variables
        }
        vb2_end(); // End the recording session
        // ===============================================================
        #endif
    }


    vb2_close(); // Close the variable buffer file and free resources
    // Cleanup and exit
    return 0;
}