

#include <var_buffer_2.h>

#ifndef TEST_RECORD_LENGTH
    #define TEST_RECORD_LENGTH 100 // Default number of records to test
#endif

int main(int, char **){
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

    double LotsOVars[100]; // Example of an array variable

    // Track the variables with their metadata
    // The type is a string that describes the type of the variable
    // Current Available types are "int", "float", "double", "long"
    vb2_track_variable(&var1, "var1", "units", "description", VB2_INT);   // int 
    vb2_track_variable(&var2, "var2", "units", "description", VB2_FLOAT); // float
    vb2_track_variable(&var3, "var3", "units", "description", VB2_DOUBLE); // double
    
    for(int i = 0; i < 100; i++) {
        char name[32];
        snprintf(name, sizeof(name), "LotsOVars[%d]", i);
        vb2_add_variable(name , "units", "description", VB2_DOUBLE, &LotsOVars[i], sizeof(LotsOVars[i]));
    }

    vb2_start(TEST_RECORD_LENGTH); // Start recording with a maximum history of 100
    vb2_record_all(); // Record all tracked variables
    for(int i = 0; i < TEST_RECORD_LENGTH; i++) {
        var1 += i; // Modify the variable to record
        var2 += 0.1f * i; // Modify the variable to record
        var3 += 0.01 * i; // Modify the variable to record
        vb2_record_all(); // Record all tracked variables
    }
    vb2_end(); // End the recording session
    vb2_close(); // Close the variable buffer file
    // Cleanup and exit
    return 0;
}