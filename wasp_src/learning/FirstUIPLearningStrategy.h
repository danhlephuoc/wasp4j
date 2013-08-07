/*
 *
 *  Copyright 2013 Mario Alviano, Carmine Dodaro, Wolfgang Faber, Nicola Leone, Francesco Ricca, and Marco Sirianni.
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

/* 
 * File:   FirstUIPLearningStrategy.h
 * Author: Carmine Dodaro
 *
 * Created on 27 July 2013, 10.17
 */

#ifndef FIRSTUIPLEARNINGSTRATEGY_H
#define	FIRSTUIPLEARNINGSTRATEGY_H

#include "LearningStrategy.h"
#include <unordered_set>
#include <list>
using namespace std;

class FirstUIPLearningStrategy : public LearningStrategy
{
    public:
        inline FirstUIPLearningStrategy( RestartsStrategy* restartsStrategy );
        
        virtual void onNavigatingLiteral( Literal* );
        virtual void onConflict( Literal* conflictLiteral, Solver& solver );        
        
    private:
        
        /**
         * This method computes the next literal to navigate in the implication graph.
         * The most recent (in the order of derivation) literal should be processed before.          
         * 
         * @return the next literal to consider.
         */
        Literal* getNextLiteralToNavigate();
        
        /**
         * This method cleans data structures.
         * It should be called in the end of each iteration.
         */
        inline void clearDataStructures();           
        
        /**
         * Add a literal in the new learned clause.
         * @param literal the literal to add.
         */
        void addLiteralInLearnedClause( Literal* literal );
        
        /**
         * The literal added by this method is a literal which should be navigated.
         * @param literal the literal to navigate.
         */
        inline void addLiteralToNavigate( Literal* literal );                
        
        /**
         * The literals already added.
         */
        unordered_set< Literal* > addedLiterals;
        
        /**
         * Literals to explore in the implication graph.
         */
        list< Literal* > literalsToNavigate;
        
        unsigned int maxDecisionLevel;
        
        unsigned int maxPosition;       
};

FirstUIPLearningStrategy::FirstUIPLearningStrategy(
    RestartsStrategy* restartsStrategy ) : LearningStrategy( restartsStrategy ), maxDecisionLevel( 0 )
{
}
        
void
FirstUIPLearningStrategy::addLiteralToNavigate( 
    Literal* literal )
{
    if( addedLiterals.insert( literal ).second )
        literalsToNavigate.push_back( literal );
}

void
FirstUIPLearningStrategy::clearDataStructures()
{
    learnedClause = NULL;
    maxDecisionLevel = 0;
    literalsToNavigate.clear();
    addedLiterals.clear();
}

#endif	/* FIRSTUIPLEARNINGSTRATEGY_H */

