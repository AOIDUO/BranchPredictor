#include "pin.H"
#include <cstdlib>
#include <fstream>
#include <iostream>
#include <math.h>
#include <bitset>

using std::cerr;
using std::endl;
using std::ios;
using std::ofstream;
using std::string;


// Simulator heartbeat rate
//
#define SIMULATOR_HEARTBEAT_INSTR_NUM 100000000 // 100m instrs


// Simulation will stop when this number of instructions have been executed
//
#define STOP_INSTR_NUM 1000000000 // 1b instrs


std::bitset<2> saturatorStrengthen(std::bitset<2> saturator ) {
    if (saturator.to_ulong() < 3) 
        return saturator.to_ulong() + 1;
    else return saturator;
}

std::bitset<2> saturatorWeaken(std::bitset<2> saturator ) {
    if (saturator.to_ulong() > 0) // strengthen local
        return saturator.to_ulong() - 1; 
    else return saturator;
}

/* Base branch predictor class */
// You are highly recommended to follow this design when implementing your
// branch predictors
//
class BranchPredictorInterface {
  public:
    // This function returns a prediction for a branch instruction with address
    // branchPC
    virtual bool getPrediction(ADDRINT branchPC) = 0;

    // This function updates branch predictor's history with outcome of branch
    // instruction with address branchPC
    virtual void train(ADDRINT branchPC, bool branchWasTaken) = 0;
};

// This is a class which implements always taken branch predictor
class AlwaysTakenBranchPredictor : public BranchPredictorInterface {
  public:
    AlwaysTakenBranchPredictor(
        UINT64 numberOfEntries){}; // no entries here: always taken branch
                                   // predictor is the simplest predictor
    virtual bool getPrediction(ADDRINT branchPC) {
        return true; // predict taken
    }
    virtual void train(ADDRINT branchPC, bool branchWasTaken) {
    } // nothing to do here: always taken branch predictor does not have history
};


class LocalBranchPredictor : public BranchPredictorInterface {
  private:
	std::vector<ADDRINT> LHR; 
	std::vector<std::bitset<2>> PHT; 
    ADDRINT lhrEntryLength;
    ADDRINT lhrLsbMask;

    int GetLhrIndex(ADDRINT branchPC){
        ADDRINT lhrIndex = branchPC & 0b1111111;
        return lhrIndex;
    }

    int GetPhtIndex(ADDRINT branchPC){
        ADDRINT lhrIndex = GetLhrIndex(branchPC);
        ADDRINT phtIndex = LHR[lhrIndex] & lhrLsbMask;
        return phtIndex;
    }

  public:
    LocalBranchPredictor(ADDRINT numberOfEntries){
        LHR = std::vector<ADDRINT>(128);
		PHT = std::vector<std::bitset<2>>(numberOfEntries);

        for (ADDRINT i = 0; i < LHR.size(); i += 1) {
            LHR[i] = 0;
        }

        lhrLsbMask = 0;
        lhrEntryLength = log2(numberOfEntries);
        for (ADDRINT i = 0; i < lhrEntryLength; i += 1) {
            lhrLsbMask = lhrLsbMask << 1;
            lhrLsbMask = lhrLsbMask | 0b1;
        }

        for (ADDRINT i = 0; i < PHT.size(); i += 1) {
            PHT[i] = 0b11;
        }
    }; 

    virtual bool getPrediction(ADDRINT branchPC) { 
        // PHT[LHR[branchPC]]
        std::bitset<2> saturator = PHT[GetPhtIndex(branchPC)];
        return saturator[1];
    } 

    virtual void train(ADDRINT branchPC, bool branchWasTaken) {
        
        ADDRINT lhrIndex = GetLhrIndex(branchPC);
        ADDRINT phtIndex = GetPhtIndex(branchPC);

        // update local history
        LHR[lhrIndex] = LHR[lhrIndex] << 1;
        LHR[lhrIndex] += branchWasTaken;

        // update saturator
        std::bitset<2> saturator = PHT[phtIndex];
        if (branchWasTaken) { // strengthen
            PHT[phtIndex] = saturatorStrengthen(saturator); 
        } else { // weaken
            PHT[phtIndex] = saturatorWeaken(saturator); 
        }

    } 
};

class GshareBranchPredictor : public BranchPredictorInterface {
  private:
	ADDRINT GHR; 
	std::vector<std::bitset<2>> PHT; 
    ADDRINT ghrEntryLength;
    ADDRINT lsbMask;

    int GetPCLsb(ADDRINT branchPC){
        ADDRINT pclsb = branchPC & lsbMask;
        return pclsb;
    }

    int GetPhtIndex(ADDRINT branchPC){
        ADDRINT pclsb = GetPCLsb(branchPC);
        ADDRINT phtIndex = (pclsb ^ GHR) & lsbMask;
        return phtIndex;
    }

  public:
    GshareBranchPredictor(ADDRINT numberOfEntries){
		PHT = std::vector<std::bitset<2>>(numberOfEntries);
        for (ADDRINT i = 0; i < PHT.size(); i += 1) {
            PHT[i] = 0b11;
        }

        ghrEntryLength = log2(numberOfEntries);
        lsbMask = 0;
        for (ADDRINT i = 0; i < ghrEntryLength; i+= 1) {
            lsbMask = lsbMask << 1;
            lsbMask = lsbMask | 0b1;
        }
    }; 

    virtual bool getPrediction(ADDRINT branchPC) { 
        // PHT[ GHR XOR branchPC]
        std::bitset<2> saturator = PHT[GetPhtIndex(branchPC)];
        return saturator[1];
    } 

    virtual void train(ADDRINT branchPC, bool branchWasTaken) {

        ADDRINT phtIndex = GetPhtIndex(branchPC);

        // update global history
        GHR = GHR << 1;
        GHR += branchWasTaken;

        // // update saturator
        std::bitset<2> saturator = PHT[phtIndex];
        if (branchWasTaken) { // strengthen
            PHT[phtIndex] = saturatorStrengthen(saturator); 

        } else { // weaken
            PHT[phtIndex] = saturatorWeaken(saturator); 
        }

    } 
};


class TournamentBranchPredictor : public BranchPredictorInterface {
  private:
	std::vector<std::bitset<2>> PHT; 
    ADDRINT lsbMask;
    LocalBranchPredictor * localPredictor;  
    GshareBranchPredictor * gsharePredictor;

    int GetPCLsb(ADDRINT branchPC){
        ADDRINT pclsb = branchPC & lsbMask;
        return pclsb;
    }

    int GetPhtIndex(ADDRINT branchPC){
        return GetPCLsb(branchPC);
    }

  public:
    TournamentBranchPredictor(ADDRINT numberOfEntries){
        localPredictor = new LocalBranchPredictor(numberOfEntries);        
        gsharePredictor = new GshareBranchPredictor(numberOfEntries);

		PHT = std::vector<std::bitset<2>>(numberOfEntries);
        for (ADDRINT i = 0; i < PHT.size(); i += 1) {
            PHT[i] = 0b11;
        }

        lsbMask = 0;
        for (ADDRINT i = 0; i < log2(numberOfEntries); i+= 1) {
            lsbMask = lsbMask << 1;
            lsbMask = lsbMask | 0b1;
        }
    }; 

    virtual bool getPrediction(ADDRINT branchPC) { 
        // PHT[ branchPC]
        std::bitset<2> saturator = PHT[GetPhtIndex(branchPC)];
        if (saturator[1] == 1){ // use gshare
            return gsharePredictor->getPrediction(branchPC);
        } else { // use local
            return localPredictor->getPrediction(branchPC);
        }
    } 

    virtual void train(ADDRINT branchPC, bool branchWasTaken) {

        ADDRINT phtIndex = GetPhtIndex(branchPC);
        std::bitset<2> saturator = PHT[phtIndex];

        // correct prediction -> meta-predictor entry is strengthened
        // mis-prediction && the unselected predictor correct -> meta-predictor entry is weakened
        // mis-prediction && both predictors wrong -> do not update meta-predictor

        // update saturator

        bool isTournamentCorrect = true;
        bool isLocalCorrect = localPredictor->getPrediction(branchPC) == branchWasTaken;
        bool isGshareCorrect = gsharePredictor->getPrediction(branchPC) == branchWasTaken;

        if (saturator[1] == 0){ // selected local
            if (isLocalCorrect) { // if local is correct
                PHT[phtIndex] = saturatorWeaken(saturator); // strengthen local
            } else {
                isTournamentCorrect = false;
            }
        } else { // selected gshare
            if (isGshareCorrect) { // if gshare is correct
                PHT[phtIndex] = saturatorStrengthen(saturator); // strengthen gshare
            } else {
                isTournamentCorrect = false; 
            }
        }

        if (!isTournamentCorrect) { // if prediction was wrong
            if (isLocalCorrect) { // if local is correct
                PHT[phtIndex] = saturatorWeaken(saturator); // strengthen local
            } else if (isGshareCorrect) { // if gshare is correct
                PHT[phtIndex] = saturatorStrengthen(saturator); // strengthen gshare
            } // else do nothing
        }

        // train gshare and local
        gsharePredictor->train(branchPC, branchWasTaken);
        localPredictor->train(branchPC, branchWasTaken);

    } 
};

ofstream OutFile;
BranchPredictorInterface *branchPredictor;

// Define the command line arguments that Pin should accept for this tool
//
KNOB<string> KnobOutputFile(KNOB_MODE_WRITEONCE, "pintool", "o", "BP_stats.out",
                            "specify output file name");
KNOB<UINT64> KnobNumberOfEntriesInBranchPredictor(
    KNOB_MODE_WRITEONCE, "pintool", "num_BP_entries", "1024",
    "specify number of entries in a branch predictor");
KNOB<string>
    KnobBranchPredictorType(KNOB_MODE_WRITEONCE, "pintool", "BP_type",
                            "always_taken",
                            "specify type of branch predictor to be used");

// The running counts of branches, predictions and instructions are kept here
//
static UINT64 iCount = 0;
static UINT64 correctPredictionCount = 0;
static UINT64 conditionalBranchesCount = 0;
static UINT64 takenBranchesCount = 0;
static UINT64 notTakenBranchesCount = 0;
static UINT64 predictedTakenBranchesCount = 0;
static UINT64 predictedNotTakenBranchesCount = 0;

VOID docount() {
    // Update instruction counter
    iCount++;
    // Print this message every SIMULATOR_HEARTBEAT_INSTR_NUM executed
    if (iCount % SIMULATOR_HEARTBEAT_INSTR_NUM == 0) {
        std::cerr << "Executed " << iCount << " instructions." << endl;
    }
    // Release control of application if STOP_INSTR_NUM instructions have been
    // executed
    if (iCount == STOP_INSTR_NUM) {
        PIN_Detach();
    }
}

VOID TerminateSimulationHandler(VOID *v) {
    OutFile.setf(ios::showbase);
    // At the end of a simulation, print counters to a file
    OutFile << "Prediction accuracy:\t"
            << (double)correctPredictionCount / (double)conditionalBranchesCount
            << endl
            << "Number of conditional branches:\t" << conditionalBranchesCount
            << endl
            << "Number of correct predictions:\t" << correctPredictionCount
            << endl
            << "Number of taken branches:\t" << takenBranchesCount << endl
            << "Number of non-taken branches:\t" << notTakenBranchesCount
            << endl;
    OutFile.close();

    std::cerr << endl
              << "PIN has been detached at iCount = " << STOP_INSTR_NUM << endl;
    std::cerr
        << endl
        << "Simulation has reached its target point. Terminate simulation."
        << endl;
    std::cerr << "Prediction accuracy:\t"
              << (double)correctPredictionCount /
                     (double)conditionalBranchesCount
              << endl;
    std::exit(EXIT_SUCCESS);
}

//
VOID Fini(int code, VOID *v) { TerminateSimulationHandler(v); }

// This function is called before every conditional branch is executed
//
static VOID AtConditionalBranch(ADDRINT branchPC, BOOL branchWasTaken) {
    /*
     * This is the place where the predictor is queried for a prediction and
     * trained
     */

    // Step 1: make a prediction for the current branch PC
    //
    bool wasPredictedTaken = branchPredictor->getPrediction(branchPC);

    // Step 2: train the predictor by passing it the actual branch outcome
    //
    branchPredictor->train(branchPC, branchWasTaken);

    // Count the number of conditional branches executed
    conditionalBranchesCount++;

    // Count the number of conditional branches predicted taken and not-taken
    if (wasPredictedTaken) {
        predictedTakenBranchesCount++;
    } else {
        predictedNotTakenBranchesCount++;
    }

    // Count the number of conditional branches actually taken and not-taken
    if (branchWasTaken) {
        takenBranchesCount++;
    } else {
        notTakenBranchesCount++;
    }

    // Count the number of correct predictions
    if (wasPredictedTaken == branchWasTaken)
        correctPredictionCount++;
}

// Pin calls this function every time a new instruction is encountered
// Its purpose is to instrument the benchmark binary so that when
// instructions are executed there is a callback to count the number of
// executed instructions, and a callback for every conditional branch
// instruction that calls our branch prediction simulator (with the PC
// value and the branch outcome).
//
VOID Instruction(INS ins, VOID *v) {
    // Insert a call before every instruction that simply counts instructions
    // executed
    INS_InsertCall(ins, IPOINT_BEFORE, (AFUNPTR)docount, IARG_END);

    // Insert a call before every conditional branch
    if (INS_IsBranch(ins) && INS_HasFallThrough(ins)) {
        INS_InsertCall(ins, IPOINT_BEFORE, (AFUNPTR)AtConditionalBranch,
                       IARG_INST_PTR, IARG_BRANCH_TAKEN, IARG_END);
    }
}

// Print Help Message
INT32 Usage() {
    cerr << "This tool simulates different types of branch predictors" << endl;
    cerr << endl << KNOB_BASE::StringKnobSummary() << endl;
    return -1;
}

int main(int argc, char *argv[]) {
    // Initialize pin
    if (PIN_Init(argc, argv))
        return Usage();

    // Create a branch predictor object of requested type
    if (KnobBranchPredictorType.Value() == "always_taken") {
        std::cerr << "Using always taken BP" << std::endl;
        branchPredictor = new AlwaysTakenBranchPredictor(
            KnobNumberOfEntriesInBranchPredictor.Value());
    } else if (KnobBranchPredictorType.Value() == "local") {
        std::cerr << "Using Local BP." << std::endl;
        branchPredictor = new LocalBranchPredictor(KnobNumberOfEntriesInBranchPredictor.Value());
    } else if (KnobBranchPredictorType.Value() == "gshare") {
        std::cerr << "Using Gshare BP." << std::endl;
           branchPredictor = new GshareBranchPredictor(KnobNumberOfEntriesInBranchPredictor.Value());
    } else if (KnobBranchPredictorType.Value() == "tournament") {
        std::cerr << "Using Tournament BP." << std::endl;
        branchPredictor = new TournamentBranchPredictor(KnobNumberOfEntriesInBranchPredictor.Value());
    } else {
        std::cerr << KnobBranchPredictorType.Value() << std::endl;
        std::cerr << "Error: No such type of branch predictor. Simulation will "
                     "be terminated."
                  << std::endl;
        std::exit(EXIT_FAILURE);
    }

    std::cerr << "The simulation will run " << STOP_INSTR_NUM
              << " instructions." << std::endl;

    OutFile.open(KnobOutputFile.Value().c_str());

    // Pin calls Instruction() when encountering each new instruction executed
    INS_AddInstrumentFunction(Instruction, 0);

    // Function to be called if the program finishes before it completes 10b
    // instructions
    PIN_AddFiniFunction(Fini, 0);

    // Callback functions to invoke before Pin releases control of the
    // application
    PIN_AddDetachFunction(TerminateSimulationHandler, 0);

    // Start the benchmark program. This call never returns...
    PIN_StartProgram();

    return 0;
}
