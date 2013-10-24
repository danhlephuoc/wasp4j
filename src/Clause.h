/*
 *
 *  Copyright 2013 Mario Alviano, Carmine Dodaro and Francesco Ricca.
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

#ifndef CLAUSE_H
#define	CLAUSE_H

#include <cassert>
#include <iostream>
#include <vector>

#include "Literal.h"
#include "Learning.h"
#include "heuristics/UndefinedCollector.h"

using namespace std;

class Solver;

/**
 *  This class represents a clause.
 *  A clause is a disjunction of literals.
 *  For example (a1 v a2 v not a3) is a clause and a1, a2 are positive literals and not a3 is a negative literal.
 * 
 */
class Clause
{
    friend ostream &operator<<( ostream & out, const Clause & clause );
    friend LearnedClause* Learning::onConflict( Literal conflictLiteral, Clause* conflictClause );

    public:
        inline Clause();
        virtual ~Clause() {}

        /**
         * Constructor of Clause which takes in input the number of literals in the clause.
         * Note that the watches are random generated.
         * 
         * @param size the numbers of literals.
         */
        inline Clause( unsigned int size );

        inline Literal getAt( unsigned idx ) const { assert( idx < literals.size() ); return literals[ idx ]; }
        inline void addLiteral( Literal literal );

        inline void attachClause();
        inline void attachClause( unsigned int firstWatch, unsigned int secondWatch );
        inline void detachClause();

        inline void onLearning( Learning* strategy );
        inline bool onLiteralFalse( Literal literal );

        inline unsigned int size() const;
        inline bool checkUnsatisfiedAndOptimize( UndefinedCollector* collector );
        bool isUnsatisfied() const;
        
        unsigned getMaxDecisionLevel( unsigned from, unsigned to) const;
        
        virtual bool isGlue() const;
        virtual bool isLearned() const;
        
        typedef vector< Literal >::iterator ClauseIterator;
        inline ClauseIterator clause_begin() { return literals.begin(); }
        inline ClauseIterator clause_end() { return literals.end(); }        

    protected:
        inline bool isImplicantOfALiteral() const;
        vector< Literal > literals;
        unsigned lastSwapIndex;

        virtual ostream& print( ostream& out ) const;

    private:
        Clause( const Clause& )
        {
            assert( "The copy constructor has been disabled." && 0 );
        }
        
        inline void setWatchesInRandomPositions();
        
        inline void attachFirstWatch();
        inline void attachSecondWatch();        
        inline void detachSecondWatch();
        
        inline bool updateWatch();
        void notifyImplication( Solver& solver );

        inline void swapLiterals( unsigned int pos1, unsigned int pos2 );
};

Clause::Clause() : lastSwapIndex( 1 )
{
}

Clause::Clause(
    unsigned int size ) : lastSwapIndex( 1 )
{
    literals.reserve( size );
}

void
Clause::addLiteral(
    Literal literal )
{
    literals.push_back( literal );
}

void
Clause::attachFirstWatch()
{
    assert( "Unary clause must be removed." && literals.size() > 1 );
    literals[ 0 ].addWatchedClause( this );
}

void
Clause::attachSecondWatch()
{
    assert( "Unary clause must be removed." && literals.size() > 1 );
    literals[ 1 ].addWatchedClause( this );
}

void
Clause::detachSecondWatch()
{
    assert( "The watchToDetach points to a NULL literal." && literals[ 1 ].getVariable() != NULL );
    literals[ 1 ].eraseWatchedClause( this );
}

void
Clause::attachClause()
{
    assert( "Unary clauses must be removed." && literals.size() > 1 );
    attachFirstWatch();
    attachSecondWatch();
}

void
Clause::attachClause( 
    unsigned int first,
    unsigned int second )
{
    assert( "First watch is out of range." && first < literals.size() );
    assert( "Second watch is out of range." && second < literals.size() );
    assert( "First watch and second watch point to the same literal." && first != second );   
    
    #ifndef NDEBUG
    Literal tmp1 = literals[ first ];
    Literal tmp2 = literals[ second ];
    #endif

    swapLiterals( 0, first );
    second == 0 ? swapLiterals( 1, first ) : swapLiterals( 1, second );    

    assert( literals[ 0 ] == tmp1 );
    assert( literals[ 1 ] == tmp2 );
    
    attachFirstWatch();
    attachSecondWatch();
}

void
Clause::detachClause()
{
    literals[ 0 ].findAndEraseWatchedClause( this );
    literals[ 1 ].findAndEraseWatchedClause( this );
}

bool
Clause::isImplicantOfALiteral() const
{
    assert( "Unary clauses must be removed." && literals.size() > 1 );

    //We assume that the literal inferred is always in the first position.
    return ( literals[ 0 ].isImplicant( this ) );
}

void
Clause::setWatchesInRandomPositions()
{
    assert( "Unary clauses must be removed." && literals.size() > 1 );
    unsigned int first = rand() % literals.size();
    unsigned int second = rand() % ( literals.size() - 1 );

    if( second >= first )
        second++;    

    assert( "First watch is not in range." && first < literals.size() );
    assert( "Second watch is not in range." && second < literals.size() );
    swapLiterals( 0, first );
    swapLiterals( 1, second );    
}

unsigned int
Clause::size() const
{
    return literals.size();
}

void
Clause::onLearning(
    Learning* strategy )
{
    assert( "LearningStrategy is not initialized." && strategy != NULL );

    //Navigating all literals in the clause.    
    for( unsigned int i = 0; i < literals.size(); i++ )
    {
        Literal literal = literals[ i ];
        strategy->onNavigatingLiteral( literal );
    }
}

bool
Clause::checkUnsatisfiedAndOptimize( 
    UndefinedCollector* collector )
{
    assert( "Unary clauses must be removed." && literals.size() > 1 );
    
    if( literals.back().isTrue() )
        return false;
    Variable* variable = literals.back().getVariable();
    if( variable->isUndefined() )
        collector->collectUndefined( variable );
    
    if( literals[ 0 ].isTrue() )
        return false;
    variable = literals[ 0 ].getVariable();
    if( variable->isUndefined() )
        collector->collectUndefined( variable );
    
    unsigned size = literals.size() - 1;
    for( unsigned int i = 1; i < size; ++i )
    {
        if( literals[ i ].isTrue() )
        {
            if( i == 1 )
                swapLiterals( 0, 1 );
            else
                swapLiterals( i, size );
            
            return false;
        }
        variable = literals[ i ].getVariable();
        if( variable->isUndefined() )
            collector->collectUndefined( variable );
    }
    
    return true;
}

void
Clause::swapLiterals( 
    unsigned int pos1, 
    unsigned int pos2 )
{
    assert( "First position is out of range." && pos1 < literals.size() );
    assert( "Second position is out of range." && pos2 < literals.size() ); 
    std::swap( literals[ pos1 ], literals[ pos2 ] );
}


bool
Clause::updateWatch()
{
    assert( "Unary clauses must be removed." && literals.size() > 1 );
    
    for( unsigned i = lastSwapIndex + 1; i < literals.size(); ++i )
    {
        if( !literals[ i ].isFalse() )
        {
            //Detach the old watch
            detachSecondWatch();

            lastSwapIndex = i;
            //Swap the two literals
            swapLiterals( 1, lastSwapIndex );

            //Attach the watch in the new position
            attachSecondWatch();            
            return true;
        }
    }
    
    for( unsigned i = 2; i <= lastSwapIndex; ++i )
    {
        if( !literals[ i ].isFalse() )
        {
            //Detach the old watch
            detachSecondWatch();

            lastSwapIndex = i;
            //Swap the two literals
            swapLiterals( 1, lastSwapIndex );

            //Attach the watch in the new position
            attachSecondWatch();            
            return true;
        }
    }

    assert( "The other watched literal cannot be true." && !literals[ 0 ].isTrue() );
    
    //Propagate literals[ 0 ];
    return false;
//    notifyImplication( solver );
}

bool
Clause::onLiteralFalse(
    Literal literal )
{
    assert( "The literal must be false." && literal.isFalse() );    
    assert( "Unary clauses must be removed." && literals.size() > 1 );

    if( literal == literals[ 0 ] )
    {
        //The watch to update should be always in position 1.
        literals[ 0 ] = literals[ 1 ];
        literals[ 1 ] = literal;
    }

    assert( "Literal is not watched." && literal == literals[ 1 ] );
    //if the clause is already satisfied do nothing.
    if( literals[ 0 ].isTrue() )
        return false;
        
    //update watch
    return !updateWatch();
}

#endif	/* CLAUSE_H */

