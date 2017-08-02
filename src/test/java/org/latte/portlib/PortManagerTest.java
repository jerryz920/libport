package org.latte.portlib;

import junit.framework.Test;
import junit.framework.TestCase;
import junit.framework.TestSuite;

/**
 * Unit test for PortManager
 */
public class PortManagerTest 
    extends TestCase
{
    /**
     * Create the test case
     *
     * @param testName name of the test case
     */
    public PortManagerTest( String testName )
    {
        super( testName );
    }

    /**
     * @return the suite of tests being tested
     */
    public static Test suite()
    {
        return new TestSuite( PortManagerTest.class );
    }

    /**
     * Rigourous Test :-)
     */
    public void testPortManager()
    {
        assertTrue( true );
    }
}
