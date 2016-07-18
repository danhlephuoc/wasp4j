/*
*
*  Copyright 2013 Mario Alviano, Carmine Dodaro, and Francesco Ricca.
*
*  Licensed under the Apache License, Version 2.0 (the "License");
*  you may not use this file except in compliance with the License.
*  You may obtain a copy of the License at
*
*    http://www.apache.org/licenses/LICENSE-2.0
*
*  Unless required by applicable law or agreed to in writing, software
*  distributed under the License is distributed on an "AS IS" BASIS,
*  WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
*  See the License for the specific language governing permissions and
*  limitations under the License.
*
*/

#include "PredicateMinimization.h"
#include "Solver.h"
#include "Clause.h"

#include <set>
using namespace std;

bool
PredicateMinimization::computeFirstModel()
{
    unsigned int result = solver.solve();
    if( result == INCOHERENT )
        return false;
    
    assert( result == COHERENT );
    return true;
}

unsigned int
PredicateMinimization::enumerationBC()
{
    solver.turnOffSimplifications();
    assert( solver.getCurrentDecisionLevel() == 0 );
    assert( !solver.conflictDetected() );
    
    if( !computeFirstModel() )
        return INCOHERENT;
    
    unsigned int min = countTrue();
    computeTrueVars();    
    if( min == 0 || !solver.addClauseFromModelAndRestart() )
    {
        printTrueVars();
        return COHERENT;
    }
    
    while( solver.solve() == COHERENT )
    {
        unsigned int count = countTrue();
        if( count < min )
        {
            computeTrueVars();
            min = count;
        }
        if( min == 0 || !solver.addClauseFromModelAndRestart() )
            break;
        
        assert( solver.getCurrentDecisionLevel() == 0 );
        assert( !solver.conflictDetected() );        
    }
    printTrueVars();
    return COHERENT;
}

unsigned int
PredicateMinimization::guessAndCheck()
{
    solver.turnOffSimplifications();
    assert( solver.getCurrentDecisionLevel() == 0 );
    assert( !solver.conflictDetected() );
    
    if( !computeFirstModel() )
        return INCOHERENT;

    if( checkAnswerSet() )
        return COHERENT;

    solver.unrollToZero();
    solver.clearConflictStatus();
    
    while( solver.solve() == COHERENT )
    {
        if( checkAnswerSet() )
            return COHERENT;
        
        solver.unrollToZero();
        solver.clearConflictStatus();
        assert( solver.getCurrentDecisionLevel() == 0 );
        assert( !solver.conflictDetected() );        
    }
    return COHERENT;
}

unsigned int
PredicateMinimization::countTrue()
{
    unsigned int count = 0;
    for( unsigned int i = 0; i < atomsToMinimize.size(); i++ )
        if( solver.isTrue( atomsToMinimize[ i ] ) )
            count++;
    return count;
}

unsigned int
PredicateMinimization::minimize()
{
    solver.copyAtomToMinimize( atomsToMinimize );
    switch( wasp::Options::predMinimizationAlgorithm )
    {
        case PREDMIN_ENUMERATION:
            return enumerationBC();
            
        case PREDMIN_GUESS_AND_CHECK:
            return guessAndCheck();
        
        case PREDMIN_GUESS_AND_CHECK_AND_MINIMIZE:
            return guessAndCheckAndMinimize();
            
        case PREDMIN_GUESS_AND_CHECK_AND_SPLIT:
            return guessAndCheckAndSplit();
            
        case NO_PREDMINIMIZATION:
        default:
            ErrorMessage::errorGeneric( "Invalid option for predicate minimization" );            
    }    
    return 0;
}

bool
PredicateMinimization::checkAnswerSet()
{
    vector< Literal > assumptions;
    Clause* clause = new Clause();
    computeTrueVars();

    for( unsigned int i = 0; i < atomsToMinimize.size(); i++ )
    {
        Var v = atomsToMinimize[ i ];
        assert( !solver.isUndefined( v ) );
        assert( !solver.hasBeenEliminated( v ) );
        
        if( solver.getDecisionLevel( v ) == 0 )
            continue;
        if( solver.isTrue( v ) )
            clause->addLiteral( Literal( v, NEGATIVE ) );
        else
        {
            clause->addLiteral( Literal( v, POSITIVE ) );
            assumptions.push_back( Literal( v, NEGATIVE ) );
        }
    }
    solver.unrollToZero();
    solver.clearConflictStatus();
    clause->setCanBeDeleted( false );
    
    bool retValue = false;
    if( !solver.addClauseRuntime( clause ) )
        retValue = true;
    else if( solver.solve( assumptions ) == INCOHERENT )
        retValue = true;
        
    if( retValue )
        printTrueVars();
    
    return retValue;  
}

unsigned int
PredicateMinimization::guessAndCheckAndMinimize()
{
    solver.turnOffSimplifications();
    assert( solver.getCurrentDecisionLevel() == 0 );
    assert( !solver.conflictDetected() );
    
    if( !computeFirstModel() )
        return INCOHERENT;

    minimizeAnswerSet();
    return COHERENT;    
}

void
PredicateMinimization::minimizeAnswerSet()
{
    vector< Var > candidates;
    vector< Literal > assumptions;
    for( unsigned int i = 0; i < atomsToMinimize.size(); i++ )
    {
        Var v = atomsToMinimize[ i ];
        assert( !solver.isUndefined( v ) );
        assert( !solver.hasBeenEliminated( v ) );
        
        if( solver.getDecisionLevel( v ) == 0 )
            continue;
        if( solver.isTrue( v ) )
            candidates.push_back( v );
        else
            assumptions.push_back( Literal( v, NEGATIVE ) );
    }
    
    begin:;
    computeTrueVars();
    
    Clause* clause = new Clause();
    for( unsigned int i = 0; i < candidates.size(); i++ )
    {
        Var v = candidates[ i ];
        assert( solver.isTrue( v ) && solver.getDecisionLevel( v ) > 0 );
        clause->addLiteral( Literal( v, NEGATIVE ) );        
    }
    
    solver.unrollToZero();
    solver.clearConflictStatus();
    clause->setCanBeDeleted( false );
    
    if( !solver.addClauseRuntime( clause ) || solver.solve( assumptions ) == INCOHERENT )
    {
        printTrueVars();
    }
    else
    {
        clause->setCanBeDeleted( true );
        unsigned int j = 0;
        for( unsigned int i = 0; i < candidates.size(); i++ )
        {
            Var v = candidates[ j ] = candidates[ i ];
            if( solver.getDecisionLevel( v ) == 0 )
                continue;
            
            if( solver.isTrue( v ) )
                j++;
            else
                assumptions.push_back( Literal( v, NEGATIVE ) );
        }
        candidates.resize( j );
        goto begin;
    }    
}

void
PredicateMinimization::computeTrueVars()
{
    trueVars.clear();
    for( unsigned int i = 1; i <= solver.numberOfVariables(); i++ )
        if( solver.isTrue( i ) )
            trueVars.push_back( i );    
}

void
PredicateMinimization::printTrueVars()
{
    OutputBuilder* output = solver.getOutputBuilder();
    output->startModel();
    for( unsigned int i = 0; i < trueVars.size(); i++ )
        output->printVariable( trueVars[ i ], true );
    output->endModel();
}

unsigned int
PredicateMinimization::guessAndCheckAndSplit()
{
    solver.turnOffSimplifications();
    assert( solver.getCurrentDecisionLevel() == 0 );
    assert( !solver.conflictDetected() );
    
    if( !computeFirstModel() )
        return INCOHERENT;

    minimizeAnswerSetSplit();
    return COHERENT; 
}

void
PredicateMinimization::minimizeAnswerSetSplit()
{
    vector< Var > candidates;
    vector< Literal > assumptions;
    for( unsigned int i = 0; i < atomsToMinimize.size(); i++ )
    {
        Var v = atomsToMinimize[ i ];
        assert( !solver.isUndefined( v ) );
        assert( !solver.hasBeenEliminated( v ) );
        
        if( solver.getDecisionLevel( v ) == 0 )
            continue;
        if( solver.isTrue( v ) )
            candidates.push_back( v );
        else
            assumptions.push_back( Literal( v, NEGATIVE ) );
    }
    
    begin:;
    if( candidates.empty() )
    {
        solver.unrollToZero();
        solver.clearConflictStatus();
        if( solver.solve( assumptions ) == INCOHERENT )
            ErrorMessage::errorGeneric( "Cannot be incoherent" );
        
        solver.printAnswerSet();
        return;
    }
    
    Var lastCandidate = candidates.back();
    candidates.pop_back();
    assumptions.push_back( Literal( lastCandidate, NEGATIVE ) );
    
    solver.unrollToZero();
    solver.clearConflictStatus();
    if( solver.solve( assumptions ) == INCOHERENT )
    {
        assumptions.pop_back();
        assumptions.push_back( Literal( lastCandidate, POSITIVE ) );
    }
    else
    {
        unsigned int j = 0;
        for( unsigned int i = 0; i < candidates.size(); i++ )
        {
            Var v = candidates[ j ] = candidates[ i ];
            if( solver.getDecisionLevel( v ) == 0 )
                continue;
            
            if( solver.isTrue( v ) )
                j++;
            else
                assumptions.push_back( Literal( v, NEGATIVE ) );
        }
        candidates.resize( j );        
    }
    goto begin;
}